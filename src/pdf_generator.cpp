#include "pdf_generator.h"
#include <QWebEnginePage>
#include <QDebug>

PdfGenerator::PdfGenerator(QObject *parent)
    : QObject(parent)
{
    page = new QWebEnginePage(this);
    connect(page, &QWebEnginePage::pdfPrintingFinished, this, &PdfGenerator::onPdfPrintingFinished);
}

PdfGenerator::~PdfGenerator()
{
}

void PdfGenerator::generatePdf(const QString &htmlContent, const QString &outputPath)
{
    // Wait for the HTML to be loaded before printing
    connect(page, &QWebEnginePage::loadFinished, this, [this, outputPath](bool ok) {
        if (ok) {
            page->printToPdf(outputPath);
        } else {
            emit finished(false, outputPath);
        }
    });

    page->setHtml(htmlContent);
}

void PdfGenerator::onPdfPrintingFinished(const QString &filePath, bool success)
{
    emit finished(success, filePath);
}
