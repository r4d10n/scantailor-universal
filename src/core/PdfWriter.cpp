/*
    Scan Tailor Universal - Interactive post-processing tool for scanned pages.
    Copyright (C) 2024

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PdfWriter.h"
#include "Dpm.h"
#include "config.h"

#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QMap>

#ifdef ENABLE_MUPDF
extern "C" {
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
}
#endif

namespace {

// Storage for multi-page PDF construction
struct PdfBuildState {
    QVector<QImage> pages;
    PdfWriter::Settings settings;
};

QMutex g_buildMutex;
QMap<QString, PdfBuildState> g_buildStates;

#ifdef ENABLE_MUPDF

// Convert QImage to a format suitable for PDF embedding
QByteArray imageToJpeg(QImage const& image, int quality)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);

    QImage img = image;
    if (img.format() == QImage::Format_Mono || img.format() == QImage::Format_MonoLSB) {
        img = img.convertToFormat(QImage::Format_Grayscale8);
    } else if (img.format() == QImage::Format_Indexed8) {
        if (img.isGrayscale()) {
            img = img.convertToFormat(QImage::Format_Grayscale8);
        } else {
            img = img.convertToFormat(QImage::Format_RGB32);
        }
    }

    img.save(&buffer, "JPEG", quality);
    return data;
}

// Create PDF document with images
bool writePdfDocument(QString const& file_path, QVector<QImage> const& images,
                      PdfWriter::Settings const& settings)
{
    if (images.isEmpty()) {
        qWarning() << "PdfWriter: No images to write";
        return false;
    }

    fz_context* ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!ctx) {
        qWarning() << "PdfWriter: Failed to create MuPDF context";
        return false;
    }

    pdf_document* doc = nullptr;
    bool success = false;

    fz_try(ctx) {
        // Create new PDF document
        doc = pdf_create_document(ctx);

        for (int i = 0; i < images.size(); ++i) {
            QImage const& image = images[i];
            if (image.isNull()) {
                continue;
            }

            // Get image dimensions and DPI
            int width = image.width();
            int height = image.height();
            float dpi_x = image.dotsPerMeterX() * 0.0254f;
            float dpi_y = image.dotsPerMeterY() * 0.0254f;
            if (dpi_x <= 0) dpi_x = 300.0f;
            if (dpi_y <= 0) dpi_y = 300.0f;

            // Calculate page size in points (72 points per inch)
            float page_width = width * 72.0f / dpi_x;
            float page_height = height * 72.0f / dpi_y;

            // Convert image to JPEG
            QByteArray jpegData = imageToJpeg(image, settings.jpegQuality);

            // Create image object from JPEG data
            fz_image* fz_img = fz_new_image_from_buffer(ctx,
                fz_new_buffer_from_copied_data(ctx,
                    (const unsigned char*)jpegData.constData(),
                    jpegData.size()));

            // Create page with the image
            fz_rect mediabox = fz_make_rect(0, 0, page_width, page_height);
            pdf_obj* resources = pdf_new_dict(ctx, doc, 1);
            pdf_obj* xobjects = pdf_new_dict(ctx, doc, 1);

            // Add image as XObject
            pdf_obj* img_ref = pdf_add_image(ctx, doc, fz_img);
            pdf_dict_puts(ctx, xobjects, "Im0", img_ref);
            pdf_dict_puts(ctx, resources, "XObject", xobjects);

            // Create content stream to draw the image
            fz_buffer* contents = fz_new_buffer(ctx, 256);
            char content_str[256];
            snprintf(content_str, sizeof(content_str),
                    "q %g 0 0 %g 0 0 cm /Im0 Do Q",
                    page_width, page_height);
            fz_append_string(ctx, contents, content_str);

            // Add page to document
            pdf_obj* page_obj = pdf_add_page(ctx, doc, mediabox, 0, resources, contents);
            pdf_insert_page(ctx, doc, -1, page_obj);

            // Cleanup
            fz_drop_image(ctx, fz_img);
            fz_drop_buffer(ctx, contents);
            pdf_drop_obj(ctx, page_obj);
        }

        // Save PDF to file
        pdf_save_document(ctx, doc, file_path.toUtf8().constData(), nullptr);
        success = true;
    }
    fz_always(ctx) {
        if (doc) {
            pdf_drop_document(ctx, doc);
        }
    }
    fz_catch(ctx) {
        qWarning() << "PdfWriter: MuPDF error:" << fz_caught_message(ctx);
        success = false;
    }

    fz_drop_context(ctx);
    return success;
}

#endif // ENABLE_MUPDF

} // anonymous namespace


bool PdfWriter::isAvailable()
{
#ifdef ENABLE_MUPDF
    return true;
#else
    return false;
#endif
}

bool PdfWriter::writeImage(QString const& file_path, QImage const& image,
                          Settings const& settings)
{
#ifdef ENABLE_MUPDF
    QVector<QImage> images;
    images.append(image);
    return writePdfDocument(file_path, images, settings);
#else
    Q_UNUSED(file_path);
    Q_UNUSED(image);
    Q_UNUSED(settings);
    qWarning() << "PdfWriter: PDF support not compiled (ENABLE_MUPDF not defined)";
    return false;
#endif
}

bool PdfWriter::writeImages(QString const& file_path, QVector<QImage> const& images,
                           Settings const& settings)
{
#ifdef ENABLE_MUPDF
    return writePdfDocument(file_path, images, settings);
#else
    Q_UNUSED(file_path);
    Q_UNUSED(images);
    Q_UNUSED(settings);
    qWarning() << "PdfWriter: PDF support not compiled (ENABLE_MUPDF not defined)";
    return false;
#endif
}

bool PdfWriter::appendImage(QString const& file_path, QImage const& image,
                           int page_no, Settings const& settings)
{
    QMutexLocker locker(&g_buildMutex);

    PdfBuildState& state = g_buildStates[file_path];
    state.settings = settings;

    // Ensure vector is large enough
    if (state.pages.size() <= page_no) {
        state.pages.resize(page_no + 1);
    }
    state.pages[page_no] = image;

    return true;
}

bool PdfWriter::finalize(QString const& file_path)
{
    QMutexLocker locker(&g_buildMutex);

    if (!g_buildStates.contains(file_path)) {
        qWarning() << "PdfWriter: No pages to finalize for" << file_path;
        return false;
    }

    PdfBuildState state = g_buildStates.take(file_path);
    locker.unlock();

    // Remove any null images (gaps in page numbers)
    QVector<QImage> validPages;
    for (const QImage& img : state.pages) {
        if (!img.isNull()) {
            validPages.append(img);
        }
    }

    if (validPages.isEmpty()) {
        qWarning() << "PdfWriter: No valid pages to write";
        return false;
    }

#ifdef ENABLE_MUPDF
    return writePdfDocument(file_path, validPages, state.settings);
#else
    Q_UNUSED(file_path);
    return false;
#endif
}
