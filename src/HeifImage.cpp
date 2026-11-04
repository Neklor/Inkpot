#include "HeifImage.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImage>

#ifdef INKPOT_HAVE_HEIF
#include <libheif/heif.h>
#endif

namespace {

QString translated(const char *text)
{
    return QCoreApplication::translate("HeifImage", text);
}

}   // namespace

bool HeifImage::isSupported()
{
#ifdef INKPOT_HAVE_HEIF
    return true;
#else
    return false;
#endif
}

bool HeifImage::looksLikeHeif(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("heic") || suffix == QLatin1String("heif")
        || suffix == QLatin1String("hif");
}

#ifdef INKPOT_HAVE_HEIF

QImage HeifImage::load(const QString &path, QString *error)
{
    const QByteArray name = QFile::encodeName(path);

    heif_context *context = heif_context_alloc();
    if (!context) {
        if (error)
            *error = translated("libheif could not be started.");
        return QImage();
    }

    heif_error result = heif_context_read_from_file(context, name.constData(), nullptr);
    if (result.code != heif_error_Ok) {
        if (error)
            *error = QString::fromUtf8(result.message);
        heif_context_free(context);
        return QImage();
    }

    heif_image_handle *handle = nullptr;
    result = heif_context_get_primary_image_handle(context, &handle);
    if (result.code != heif_error_Ok) {
        if (error)
            *error = QString::fromUtf8(result.message);
        heif_context_free(context);
        return QImage();
    }

    // Interleaved RGBA matches QImage::Format_RGBA8888 byte for byte, so the
    // picture only has to be copied once. libheif applies the rotation stored
    // in the file on its own, so photographs come out upright.
    heif_image *decoded = nullptr;
    result = heif_decode_image(handle, &decoded, heif_colorspace_RGB,
                               heif_chroma_interleaved_RGBA, nullptr);
    if (result.code != heif_error_Ok) {
        if (error)
            *error = QString::fromUtf8(result.message);
        heif_image_handle_release(handle);
        heif_context_free(context);
        return QImage();
    }

    int stride = 0;
    const uint8_t *plane = heif_image_get_plane_readonly(decoded, heif_channel_interleaved,
                                                         &stride);
    const int width = heif_image_get_width(decoded, heif_channel_interleaved);
    const int height = heif_image_get_height(decoded, heif_channel_interleaved);

    QImage image;
    if (plane && width > 0 && height > 0) {
        image = QImage(plane, width, height, stride, QImage::Format_RGBA8888).copy();
    } else if (error) {
        *error = translated("The file holds no picture.");
    }

    heif_image_release(decoded);
    heif_image_handle_release(handle);
    heif_context_free(context);
    return image;
}

#else

QImage HeifImage::load(const QString &path, QString *error)
{
    Q_UNUSED(path);
    if (error)
        *error = translated("This build has no support for HEIF pictures.");
    return QImage();
}

#endif
