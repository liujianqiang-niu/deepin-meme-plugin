// SPDX-License-Identifier: GPL-3.0-or-later
#include "memeplugin.h"

#include "dccfactory.h"

#include <DConfig>

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QFile>
#include <QProcess>

DCORE_USE_NAMESPACE

Q_LOGGING_CATEGORY(memePlugin, "meme.plugin")

static const char *kAppId = "org.deepin.meme";
static const char *kPresetDir = "/usr/share/deepin-meme-wallpapers";

MemePlugin::MemePlugin(QObject *parent)
    : QObject(parent)
    , m_model(new WallpaperModel(this))
    , m_converter(new VideoConverter(this))
{
    readConfig();

    connect(m_converter, &VideoConverter::progress, this, [this](int pct) {
        m_convertProgress = pct;
        emit convertProgressChanged();
    });
    connect(m_converter, &VideoConverter::finished, this, [this](bool success, const QString &out, const QString &err) {
        m_convertProgress = 0;
        emit convertProgressChanged();
        emit convertingChanged();

        if (success) {
            qCInfo(memePlugin) << "convert success:" << out;
            m_model->refresh();
            // 不自动应用壁纸，让用户自己从列表中选择并点"应用"
            // 之前自动 applyWallpaper 导致 edge 插件在文件刚写入时打开失败
        } else {
            qCWarning(memePlugin) << "convert failed:" << err;
        }
    });
}

MemePlugin::~MemePlugin() = default;

void MemePlugin::readConfig()
{
    DConfig *cfg = DConfig::create(kAppId, kAppId, QString(), this);
    if (!cfg || !cfg->isValid()) {
        qCWarning(memePlugin) << "DConfig not valid";
        return;
    }
    m_enabled = cfg->value("enabled", false).toBool();
    m_currentVideo = cfg->value("currentVideo", QString()).toString();
    cfg->deleteLater();
}

void MemePlugin::writeConfigEnabled(bool e)
{
    DConfig *cfg = DConfig::create(kAppId, kAppId, QString(), this);
    if (cfg && cfg->isValid()) {
        cfg->setValue("enabled", e);
        cfg->deleteLater();
    }
}

void MemePlugin::writeConfigCurrentVideo(const QString &path)
{
    DConfig *cfg = DConfig::create(kAppId, kAppId, QString(), this);
    if (cfg && cfg->isValid()) {
        cfg->setValue("currentVideo", path);
        cfg->deleteLater();
    }
}

bool MemePlugin::enabled() const { return m_enabled; }

void MemePlugin::setEnabled(bool e)
{
    if (m_enabled != e) {
        m_enabled = e;
        emit enabledChanged(e);
        writeConfigEnabled(e);
        // edge 插件监听 DConfig valueChanged 自动启停，无需 D-Bus
    }
}

QString MemePlugin::currentVideo() const { return m_currentVideo; }

void MemePlugin::setCurrentVideo(const QString &path)
{
    if (m_currentVideo != path) {
        m_currentVideo = path;
        emit currentVideoChanged(path);
        writeConfigCurrentVideo(path);
    }
}

WallpaperModel *MemePlugin::wallpaperModel() const { return m_model; }

bool MemePlugin::converting() const { return m_converter->isConverting(); }

int MemePlugin::convertProgress() const { return m_convertProgress; }

void MemePlugin::applyWallpaper(const QString &path)
{
    qCInfo(memePlugin) << "applyWallpaper:" << path;
    setCurrentVideo(path);
    if (!m_enabled) {
        m_enabled = true;
        emit enabledChanged(true);
        writeConfigEnabled(true);
    }
}

QUrl MemePlugin::urlFromPath(const QString &path) const
{
    return QUrl::fromLocalFile(path);
}

void MemePlugin::uploadVideo(const QUrl &url)
{
    const QString localPath = url.toLocalFile();
    if (localPath.isEmpty()) {
        qCWarning(memePlugin) << "uploadVideo: invalid url" << url;
        return;
    }

    const QString userDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/deepin-meme-wallpapers");
    QDir().mkpath(userDir);

    // 检查格式：H264 直接复制，非 H264 转码
    if (VideoConverter::checkFormat(localPath)) {
        const QString dest = QDir(userDir).filePath(QFileInfo(localPath).fileName());
        if (QFile::exists(dest))
            QFile::remove(dest);
        if (QFile::copy(localPath, dest)) {
            qCInfo(memePlugin) << "copied H264 video:" << dest;
            m_model->refresh();
        } else {
            qCWarning(memePlugin) << "copy failed:" << dest;
        }
    } else {
        qCInfo(memePlugin) << "starting conversion for" << localPath;
        m_convertProgress = 0;
        emit convertingChanged();
        emit convertProgressChanged();
        m_converter->convert(localPath, userDir);
    }
}

void MemePlugin::removeUserWallpaper(int index)
{
    const QString path = m_model->pathAt(index);
    m_model->removeUserWallpaper(index);
    if (path == m_currentVideo) {
        setCurrentVideo(QString());
        if (m_enabled) {
            m_enabled = false;
            emit enabledChanged(false);
            writeConfigEnabled(false);
        }
    }
}

void MemePlugin::cancelConvert()
{
    m_converter->cancel();
    m_convertProgress = 0;
    emit convertProgressChanged();
    emit convertingChanged();
}

DCC_FACTORY_CLASS(MemePlugin)
#include "memeplugin.moc"
