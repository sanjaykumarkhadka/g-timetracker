#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>


#include "DataExporter.h"
#include "TimeLogHistory.h"


#define fail(message) \
    do {    \
        qCCritical(DATA_IO_CATEGORY) << message;    \
        QCoreApplication::exit(EXIT_FAILURE);   \
    } while (0)

DataExporter::DataExporter(TimeLogHistory *db, QObject *parent) :
    AbstractDataInOut(db, parent)
{
    connect(m_db, SIGNAL(historyRequestCompleted(QVector<TimeLogEntry>,qlonglong)),
            this, SLOT(historyRequestCompleted(QVector<TimeLogEntry>,qlonglong)));
    connect(m_db, SIGNAL(storedCategoriesAvailable(QVector<TimeLogCategory>)),
            this, SLOT(storedCategoriesAvailable(QVector<TimeLogCategory>)));
}

void DataExporter::startIO(const QString &path)
{
    if (!prepareDir(path, m_dir)) {
        fail(QString("Fail to prepare directory %1").arg(path));
        return;
    } else if (!m_dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        fail(QString("Target directory %1 is not empty").arg(m_dir.path()));
        return;
    }

    if (!m_db->size()) {
        qCInfo(DATA_IO_CATEGORY) << "No data to export";
        QCoreApplication::quit();
        return;
    }

    m_db->getStoredCategories();
}

void DataExporter::historyError(const QString &errorText)
{
    fail(QString("Fail to get data from db: %1").arg(errorText));
}

void DataExporter::historyRequestCompleted(QVector<TimeLogEntry> data, qlonglong id)
{
    if (id == 0) {          // Begin of export period received
        m_currentDate = data.first().startTime.toUTC().date();
        m_db->getHistoryBefore(1, 1, QDateTime::currentDateTimeUtc());
        return;
    } else if (id == 1) {   // End of export period received
        m_lastDate = data.last().startTime.toUTC().date();
    } else if (m_currentDate > m_lastDate) {
        qCInfo(DATA_IO_CATEGORY) << "All data successfully exported";
        QCoreApplication::quit();
        return;
    } else {
        if (!exportDay(data)) {
            return;
        }
        m_currentDate = m_currentDate.addDays(1);
    }

    exportCurrentDate(++id);
}

void DataExporter::storedCategoriesAvailable(QVector<TimeLogCategory> data)
{
    if (!exportCategories(data)) {
        return;
    }

    m_db->getHistoryAfter(0, 1, QDateTime::fromTime_t(0, Qt::UTC));
}

void DataExporter::exportCurrentDate(qlonglong id)
{
    m_db->getHistoryBetween(id, QDateTime(m_currentDate, QTime(), Qt::UTC),
                            QDateTime(m_currentDate.addDays(1), QTime(), Qt::UTC).addSecs(-1));
}

bool DataExporter::exportDay(const QVector<TimeLogEntry> &data)
{
    if (data.isEmpty()) {
        qCInfo(DATA_IO_CATEGORY) << QString("No data for date %1").arg(m_currentDate.toString());
        return true;
    }

    qCInfo(DATA_IO_CATEGORY) << QString("Exporting data for date %1").arg(m_currentDate.toString());

    QString fileName = QString("%1 (%2).csv").arg(m_currentDate.toString(Qt::ISODate))
                                             .arg(m_currentDate.toString("ddd"));
    QString filePath = m_dir.filePath(fileName);
    QFile file(filePath);
    if (file.exists()) {
        fail(QString("File %1 already exists").arg(filePath));
        return false;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fail(formatFileError("Fail to open file", file));
        return false;
    }

    QStringList strings;
    foreach (const TimeLogEntry &entry, data) {
        QStringList values;
        values << entry.startTime.toUTC().toString(Qt::ISODate);
        values << entry.category;
        values << entry.comment;
        values << entry.uuid.toString();
        strings.append(values.join(m_sep));
    }

    QTextStream stream(&file);
    stream << strings.join('\n') << endl;
    if (stream.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
        fail(formatFileError("Error writing to file", file));
        return false;
    }

    return true;
}

bool DataExporter::exportCategories(const QVector<TimeLogCategory> &data)
{
    if (data.isEmpty()) {
        qCInfo(DATA_IO_CATEGORY) << QString("No categories");
        return true;
    }

    qCInfo(DATA_IO_CATEGORY) << QString("Exporting categories");

    QString fileName("categories.csv");
    QString filePath = m_dir.filePath(fileName);
    QFile file(filePath);
    if (file.exists()) {
        fail(QString("File %1 already exists").arg(filePath));
        return false;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fail(formatFileError("Fail to open file", file));
        return false;
    }

    QStringList strings;
    foreach (const TimeLogCategory &category, data) {
        QStringList values;
        values << category.name;
        values << QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(category.data)).toJson(QJsonDocument::Compact));
        values << category.uuid.toString();
        strings.append(values.join(m_sep));
    }

    QTextStream stream(&file);
    stream << strings.join('\n') << endl;
    if (stream.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
        fail(formatFileError("Error writing to file", file));
        return false;
    }

    return true;
}
