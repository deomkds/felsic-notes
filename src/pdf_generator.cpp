#include "pdf_generator.h"
#include <QWebEnginePage>
#include <QDebug>
#include <QPageLayout>
#include <QPageSize>
#include <QMarginsF>

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
            QPageLayout layout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
            page->printToPdf(outputPath, layout);
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
