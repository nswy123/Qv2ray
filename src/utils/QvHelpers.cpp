#include "QvHelpers.hpp"
#include "QvUtils.hpp"
#include <QQueue>

// Forwarded from QvTinyLog
static QQueue<QString> __loggerBuffer;

void __QV2RAY_LOG_FUNC__(int type, const std::string &func, int line, const QString &module, const QString &log)
{
    auto logString = "[" + module + "]: " + log;
    auto funcPrepend = QString::fromStdString(func + ":" + to_string(line) + " ");

    if (isDebug) {
        // Debug build version, we only print info for DEBUG logs and print ALL info when debugLog presents,
        if (type == QV2RAY_LOG_DEBUG || StartupOption.debugLog) {
            logString = logString.prepend(funcPrepend);
        }
    } else {
        // We only process DEBUG log in Release mode
        if (type == QV2RAY_LOG_DEBUG) {
            if (StartupOption.debugLog) {
                logString = logString.prepend(funcPrepend);
            } else {
                // Discard debug log in non-debug Qv2ray version with no-debugLog mode.
                return;
            }
        }
    }

    cout << logString.toStdString() << endl;
    __loggerBuffer.enqueue(logString + NEWLINE);
}

const QString readLastLog()
{
    QString result;

    while (!__loggerBuffer.isEmpty()) {
        auto str = __loggerBuffer.dequeue();

        if (!str.trimmed().isEmpty()) {
            result += str;
        }
    }

    return result;
}

namespace Qv2ray
{
    namespace Utils
    {
        const QString GenerateRandomString(int len)
        {
            const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
            QString randomString;

            for (int i = 0; i < len; ++i) {
                uint rand = QRandomGenerator::system()->generate();
                uint max = static_cast<uint>(possibleCharacters.length());
                QChar nextChar = possibleCharacters[rand % max];
                randomString.append(nextChar);
            }

            return randomString;
        }

        QString StringFromFile(QFile *source)
        {
            source->open(QFile::ReadOnly);
            QTextStream stream(source);
            QString str = stream.readAll();
            source->close();
            return str;
        }

        bool StringToFile(const QString *text, QFile *targetFile)
        {
            bool override = targetFile->exists();
            targetFile->open(QFile::WriteOnly);
            QTextStream stream(targetFile);
            stream << *text << endl;
            stream.flush();
            targetFile->close();
            return override;
        }

        QJsonObject JSONFromFile(QFile *sourceFile)
        {
            QString json = StringFromFile(sourceFile);
            return JsonFromString(json);
        }

        QString JsonToString(QJsonObject json, QJsonDocument::JsonFormat format)
        {
            QJsonDocument doc;
            doc.setObject(json);
            return doc.toJson(format);
        }

        QString JsonToString(QJsonArray array, QJsonDocument::JsonFormat format)
        {
            QJsonDocument doc;
            doc.setArray(array);
            return doc.toJson(format);
        }

        QString VerifyJsonString(const QString &source)
        {
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(source.toUtf8(), &error);
            Q_UNUSED(doc)

            if (error.error == QJsonParseError::NoError) {
                return "";
            } else {
                LOG(MODULE_UI, "WARNING: Json parse returns: " + error.errorString())
                return error.errorString();
            }
        }

        QJsonObject JsonFromString(QString string)
        {
            QJsonDocument doc = QJsonDocument::fromJson(string.toUtf8());
            return doc.object();
        }

        QString Base64Encode(QString string)
        {
            QByteArray ba = string.toUtf8();
            return ba.toBase64();
        }

        QString Base64Decode(QString string)
        {
            QByteArray ba = string.toUtf8();
            return QString(QByteArray::fromBase64(ba));
        }

        QStringList SplitLines(const QString &_string)
        {
            return _string.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
        }

        list<string> SplitLines_std(const QString &_string)
        {
            list<string> list;

            for (auto line : _string.split(QRegExp("[\r\n]"), QString::SkipEmptyParts)) {
                list.push_back(line.toStdString());
            }

            return list;
        }

        QStringList GetFileList(QDir dir)
        {
            return dir.entryList(QStringList() << "*" << "*.*", QDir::Hidden | QDir::Files);
        }

        bool FileExistsIn(QDir dir, QString fileName)
        {
            return GetFileList(dir).contains(fileName);
        }

        void QvMessageBoxWarn(QWidget *parent, QString title, QString text)
        {
            QMessageBox::warning(parent, title, text, QMessageBox::Ok | QMessageBox::Default, 0);
        }

        void QvMessageBoxInfo(QWidget *parent, QString title, QString text)
        {
            QMessageBox::information(parent, title, text, QMessageBox::Ok | QMessageBox::Default, 0);
        }


        int QvMessageBoxAsk(QWidget *parent, QString title, QString text, QMessageBox::StandardButton extraButtons)
        {
            return QMessageBox::question(parent, title, text, QMessageBox::Yes | QMessageBox::No | extraButtons);
        }

        QString FormatBytes(long long bytes)
        {
            char str[64];
            const char *sizes[5] = { "B", "KB", "MB", "GB", "TB" };
            int i;
            double dblByte = bytes;

            for (i = 0; i < 5 && bytes >= 1024; i++, bytes /= 1024)
                dblByte = bytes / 1024.0;

            sprintf(str, "%.2f", dblByte);
            return strcat(strcat(str, " "), sizes[i]);
        }

        bool IsValidFileName(const QString &fileName)
        {
            QString name = fileName;
            return name == RemoveInvalidFileName(fileName);
        }

        QString RemoveInvalidFileName(const QString &fileName)
        {
            std::string _name = fileName.toStdString();
            std::replace_if(_name.begin(), _name.end(), [](char c) {
                return std::string::npos != string(R"("/\?%&^*;:|><)").find(c);
            }, '_');
            return QString::fromStdString(_name);
        }

        QTranslator *getTranslator(const QString &lang)
        {
            QTranslator *translator = new QTranslator();
            translator->load(lang + ".qm", ":/translations/");
            return translator;
        }

        /// This returns a file name without extensions.
        void DeducePossibleFileName(const QString &baseDir, QString *fileName, const QString &extension)
        {
            int i = 1;

            if (!QDir(baseDir).exists()) {
                QDir(baseDir).mkpath(baseDir);
                LOG(MODULE_FILE, "Making path: " + baseDir)
            }

            while (true) {
                if (!QFile(baseDir + "/" + fileName + "_" + QSTRN(i) + extension).exists()) {
                    *fileName = *fileName + "_" + QSTRN(i);
                    return;
                } else {
                    DEBUG(MODULE_FILE, "File with name: " + *fileName + "_" + QSTRN(i) + extension + " already exists")
                }

                i++;
            }
        }

        void QFastAppendTextDocument(const QString &message, QTextDocument *doc)
        {
            QTextCursor cursor(doc);
            cursor.movePosition(QTextCursor::End);
            cursor.beginEditBlock();
            cursor.insertBlock();
            cursor.insertHtml(message);
            cursor.endEditBlock();
        }

        QStringList ConvertQStringList(const QList<string> &stdListString)
        {
            QStringList listQt;
            listQt.reserve(stdListString.size());

            for (const std::string &s : stdListString) {
                listQt.append(QString::fromStdString(s));
            }

            return listQt;
        }
        std::list<string> ConvertStdStringList(const QStringList &qStringList)
        {
            std::list<string> stdList;

            for (auto &s : qStringList) {
                stdList.push_back(s.toStdString());
            }

            return stdList;
        }
    }
}
