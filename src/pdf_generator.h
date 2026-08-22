#ifndef PDF_GENERATOR_H
#define PDF_GENERATOR_H

#include <QObject>
#include <QString>
#include <QWebEnginePage>

class PdfGenerator : public QObject
{
    Q_OBJECT

public:
    explicit PdfGenerator(QObject *parent = nullptr);
    ~PdfGenerator();

    void generatePdf(const QString &htmlContent, const QString &outputPath);

signals:
    void finished(bool success, const QString &outputPath);

private slots:
    void onPdfPrintingFinished(const QString &filePath, bool success);

private:
    QWebEnginePage *page;
};

#endif // PDF_GENERATOR_H
