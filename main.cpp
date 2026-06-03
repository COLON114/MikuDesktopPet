#include <QApplication>
#include <QLabel>
#include <QMovie>
#include <QWidget>
#include <QMouseEvent>
#include <QPoint>
#include <QDebug>
#include <QDir>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ========== DeepSeek API 配置 ==========
QJsonObject loadConfig() 
{
    QFile file("config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开 config.json，请检查文件是否存在";
        return QJsonObject();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return doc.object();
}

const QString DEEPSEEK_API_KEY = loadConfig()["deepseek_api_key"].toString();
const QString DEEPSEEK_API_URL = loadConfig()["deepseek_api_url"].toString();

// Miku 的性格设定
const QString MIKU_SYSTEM_PROMPT = 
    "你是初音未来（Hatsune Miku），一个16岁的虚拟歌姬。"
    "你的性格是活泼、可爱、善良、有点傲娇。"
    "你说话的语气要像朋友一样亲切，偶尔加上'～'、'♪'等可爱的语气词。"
    "回答要简短自然，不要太长，像真人聊天一样。"
    "你会关心用户，但不会过分啰嗦。"
    "如果用户问你不知道的问题，就可爱地承认自己不知道。";

// ========== 自定义输入框 ==========
class CustomInputDialog : public QFrame {
    Q_OBJECT
public:
    CustomInputDialog(QWidget *parent = nullptr) : QFrame(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        
        QFrame *bgFrame = new QFrame(this);
        bgFrame->setStyleSheet(
            "QFrame {"
            "  background-color: rgba(30, 30, 40, 0.9);"
            "  border-radius: 15px;"
            "  border: 1px solid rgba(255, 255, 255, 0.2);"
            "}"
        );
        QVBoxLayout *bgLayout = new QVBoxLayout(bgFrame);
        bgLayout->setSpacing(8);
        
        QLabel *promptLabel = new QLabel("🎤 对 Miku 说点什么吧", bgFrame);
        promptLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 12px; font-weight: bold; }");
        bgLayout->addWidget(promptLabel);
        
        inputEdit = new QLineEdit(bgFrame);
        inputEdit->setPlaceholderText("输入你的问题...");
        inputEdit->setStyleSheet(
            "QLineEdit {"
            "  background-color: rgba(255, 255, 255, 0.15);"
            "  border: 1px solid rgba(255, 255, 255, 0.3);"
            "  border-radius: 8px;"
            "  padding: 8px 10px;"
            "  color: #ffffff;"
            "  font-size: 13px;"
            "}"
            "QLineEdit:focus { border: 1px solid rgba(0, 200, 255, 0.8); }"
        );
        bgLayout->addWidget(inputEdit);
        
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(10);
        
        sendBtn = new QPushButton("发送", bgFrame);
        cancelBtn = new QPushButton("取消", bgFrame);
        
        sendBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(0, 200, 255, 0.6);"
            "  border: none; border-radius: 6px;"
            "  padding: 5px 15px; color: white; font-size: 12px;"
            "}"
            "QPushButton:hover { background-color: rgba(0, 200, 255, 0.9); }"
        );
        cancelBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(100, 100, 120, 0.6);"
            "  border: none; border-radius: 6px;"
            "  padding: 5px 15px; color: white; font-size: 12px;"
            "}"
            "QPushButton:hover { background-color: rgba(100, 100, 120, 0.9); }"
        );
        
        btnLayout->addStretch();
        btnLayout->addWidget(sendBtn);
        btnLayout->addWidget(cancelBtn);
        btnLayout->addStretch();
        
        bgLayout->addLayout(btnLayout);
        mainLayout->addWidget(bgFrame);
        
        connect(sendBtn, &QPushButton::clicked, this, &CustomInputDialog::onSend);
        connect(cancelBtn, &QPushButton::clicked, this, &CustomInputDialog::close);
        connect(inputEdit, &QLineEdit::returnPressed, this, &CustomInputDialog::onSend);
        
        setFocusProxy(inputEdit);
        setFixedSize(280, 110);
    }
    
    QString getText() const { return inputEdit->text(); }
    void clear() { inputEdit->clear(); }
    
    void showAt(const QPoint &globalPos) {
        move(globalPos);
        inputEdit->clear();
        inputEdit->setFocus();
        show();
        raise();
    }
    
signals:
    void textEntered(const QString &text);
    
private slots:
    void onSend() {
        QString text = inputEdit->text().trimmed();
        if (!text.isEmpty()) {
            emit textEntered(text);
        }
        close();
    }
    
private:
    QLineEdit *inputEdit;
    QPushButton *sendBtn;
    QPushButton *cancelBtn;
};

// ========== 自定义气泡框 ==========
class CustomBubble : public QFrame {
    Q_OBJECT
public:
    CustomBubble(const QString &text, QWidget *parent = nullptr) : QFrame(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 8, 10, 8);
        
        QLabel *label = new QLabel(text, this);
        label->setWordWrap(true);
        label->setMaximumWidth(280);
        label->setStyleSheet("QLabel { color: #ffffff; font-size: 13px; }");
        
        QFrame *bubbleFrame = new QFrame(this);
        bubbleFrame->setStyleSheet(
            "QFrame {"
            "  background-color: rgba(30, 30, 40, 0.95);"
            "  border-radius: 12px;"
            "  border: 1px solid rgba(255, 255, 255, 0.2);"
            "}"
        );
        QVBoxLayout *bubbleLayout = new QVBoxLayout(bubbleFrame);
        bubbleLayout->addWidget(label);
        layout->addWidget(bubbleFrame);
        
        adjustSize();
        QTimer::singleShot(4000, this, &QWidget::close);
    }
    
    void showAt(const QPoint &pos) {
        move(pos);
        show();
        raise();
    }
};

// ========== Miku 主窗口 ==========
class MikuWidget : public QWidget {
    Q_OBJECT
public:
    MikuWidget(QWidget *parent = nullptr) : QWidget(parent), inputDialog(nullptr) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        
        label = new QLabel(this);
        movie = new QMovie("resources/miku.gif");
        
        if (movie->isValid()) {
            label->setMovie(movie);
            movie->start();
            label->resize(movie->currentPixmap().size());
            resize(label->size());
        } else {
            resize(300, 400);
        }
        
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        audioOutput->setVolume(0.8);
        
        networkManager = new QNetworkAccessManager(this);
        hasMoved = false;
        
        inputDialog = new CustomInputDialog(nullptr);
        connect(inputDialog, &CustomInputDialog::textEntered, this, &MikuWidget::onUserInput);
    }
    
    ~MikuWidget() {
        if (inputDialog) {
            inputDialog->close();
            inputDialog->deleteLater();
        }
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            dragStartPosition = event->globalPosition().toPoint();
            dragPosition = dragStartPosition - frameGeometry().topLeft();
            hasMoved = false;
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::LeftButton) {
            move(event->globalPosition().toPoint() - dragPosition);
            hasMoved = true;
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            if (!hasMoved) {
                // 左键单击弹出输入框
                showInputDialog();
            }
            event->accept();
        }
        else if (event->button() == Qt::RightButton) {
            // 右键单击播放音效
            QString soundPath = "resources/sounds/sound1.wav";
            if (QFile::exists(soundPath)) {
                player->setSource(QUrl::fromLocalFile(soundPath));
                player->play();
            }
            event->accept();
        }
    }

private:
    void showInputDialog() {
        if (inputDialog) {
            QPoint mikuPos = mapToGlobal(QPoint(width(), 0));
            inputDialog->showAt(QPoint(mikuPos.x() + 10, mikuPos.y() + 50));
        }
    }
    
    void onUserInput(const QString &text) {
        showBubble("💬 " + text);
        callDeepSeekAPI(text);
    }
    
    void callDeepSeekAPI(const QString &userMessage) {
        QNetworkRequest request;
        request.setUrl(QUrl(DEEPSEEK_API_URL));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(DEEPSEEK_API_KEY).toUtf8());
        
        QJsonObject body;
        body["model"] = "deepseek-chat";
        body["temperature"] = 0.8;
        body["max_tokens"] = 150;
        
        QJsonArray messages;
        
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = MIKU_SYSTEM_PROMPT;
        messages.append(systemMsg);
        
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = userMessage;
        messages.append(userMsg);
        
        body["messages"] = messages;
        
        QNetworkReply *reply = networkManager->post(request, QJsonDocument(body).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            handleApiResponse(reply);
        });
    }
    
    void handleApiResponse(QNetworkReply *reply) {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "API 错误:" << reply->errorString();
            showBubble("😖 网络出错了... 再试一次吧～");
            reply->deleteLater();
            return;
        }
        
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        QString replyText;
        if (doc["choices"].isArray() && doc["choices"].toArray().size() > 0) {
            replyText = doc["choices"][0]["message"]["content"].toString();
        } else {
            replyText = "唔... 我没听清楚，能再说一次吗～";
        }
        
        qDebug() << "Miku 回复:" << replyText;
        
        if (currentBubble) {
            currentBubble->close();
            currentBubble->deleteLater();
        }
        showBubble("🎤 " + replyText);
        
        reply->deleteLater();
    }
    
    void showBubble(const QString &text) {
        if (currentBubble) {
            currentBubble->close();
            currentBubble->deleteLater();
        }
        
        currentBubble = new CustomBubble(text);
        QPoint mikuPos = mapToGlobal(QPoint(0, 0));
        int bubbleX = mikuPos.x() + width() / 2 - 120;
        int bubbleY = mikuPos.y() - 70;
        currentBubble->showAt(QPoint(bubbleX, bubbleY));
    }

private:
    QLabel *label;
    QMovie *movie;
    QPoint dragStartPosition;
    QPoint dragPosition;
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QNetworkAccessManager *networkManager;
    bool hasMoved;
    CustomInputDialog *inputDialog;
    CustomBubble *currentBubble = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MikuWidget miku;
    miku.show();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon *trayIcon = new QSystemTrayIcon(&miku);
        QIcon icon("resources/icon.png");
        if (!icon.isNull()) trayIcon->setIcon(icon);
        trayIcon->setToolTip("Miku 桌面宠物");
        
        QMenu *trayMenu = new QMenu();
        QAction *showAction = new QAction("显示 Miku", trayMenu);
        QAction *hideAction = new QAction("隐藏 Miku", trayMenu);
        QAction *quitAction = new QAction("退出", trayMenu);
        
        trayMenu->addAction(showAction);
        trayMenu->addAction(hideAction);
        trayMenu->addSeparator();
        trayMenu->addAction(quitAction);
        
        QObject::connect(showAction, &QAction::triggered, [&miku]() { miku.show(); });
        QObject::connect(hideAction, &QAction::triggered, [&miku]() { miku.hide(); });
        QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);
        
        trayIcon->setContextMenu(trayMenu);
        trayIcon->show();
    }

    return app.exec();
}

#include "main.moc"