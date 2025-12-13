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

#ifndef PDFWRITER_H_
#define PDFWRITER_H_

#include <QString>
#include <QImage>
#include <QVector>

class Dpm;

/**
 * @brief PDF writer class for creating PDF documents from images
 *
 * Supports single-page and multi-page PDF creation using MuPDF library.
 * Images are embedded as JPEG (for color/grayscale) or CCITT G4 (for B&W).
 */
class PdfWriter
{
public:
    /**
     * @brief Compression/quality settings for PDF output
     */
    struct Settings {
        int jpegQuality;      ///< JPEG quality for color images (1-100, default 85)
        bool useJpegForGray;  ///< Use JPEG for grayscale (vs. Deflate)
        bool forceJpeg;       ///< Force JPEG even for B&W (larger file)

        Settings() : jpegQuality(85), useJpegForGray(true), forceJpeg(false) {}
    };

    /**
     * @brief Writes a single QImage to a PDF file
     *
     * @param file_path The full path to the output PDF file
     * @param image The image to write (must not be null)
     * @param settings Optional compression settings
     * @return True on success, false on failure
     */
    static bool writeImage(QString const& file_path, QImage const& image,
                          Settings const& settings = Settings());

    /**
     * @brief Writes multiple images to a multi-page PDF file
     *
     * @param file_path The full path to the output PDF file
     * @param images Vector of images (one per page)
     * @param settings Optional compression settings
     * @return True on success, false on failure
     */
    static bool writeImages(QString const& file_path, QVector<QImage> const& images,
                           Settings const& settings = Settings());

    /**
     * @brief Appends an image to an existing PDF or creates a new one
     *
     * For multi-page PDF creation in a streaming fashion.
     *
     * @param file_path The full path to the PDF file
     * @param image The image to append
     * @param page_no Page number (0-based, used for ordering)
     * @param settings Optional compression settings
     * @return True on success, false on failure
     */
    static bool appendImage(QString const& file_path, QImage const& image,
                           int page_no, Settings const& settings = Settings());

    /**
     * @brief Finalizes a multi-page PDF after all pages have been appended
     *
     * Call this after all appendImage() calls to write the final PDF.
     *
     * @param file_path The full path to the PDF file
     * @return True on success, false on failure
     */
    static bool finalize(QString const& file_path);

    /**
     * @brief Check if PDF writing is available (MuPDF enabled)
     * @return true if PDF export is supported
     */
    static bool isAvailable();

private:
    static void setDpm(void* page, Dpm const& dpm);
};

#endif // PDFWRITER_H_
