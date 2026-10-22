#include "noise.h"

static const int gaussian3x3[3][3] = {{1,2,1},{2,4,2},{1,2,1}};

noise::noise()
{

}

void noise::smoothing(Filters filter, QImage image){
    // Work on a 1-byte-per-pixel copy and walk it with scanLine(): Qt pads every
    // row to a multiple of 4 bytes, so "x + y * width" is not a valid index in general.
    // The old code also created the output in the source format (4 bytes per pixel)
    // and wrote 1 byte per pixel into it, which left most of the image black.
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    const int w = gray.width();
    const int h = gray.height();

    // Start from a copy so the 1-pixel border keeps the original values
    qDenoisingImage = gray.copy();

    for (int y = 1; y < h - 1; y++) {
        const uchar* up   = gray.constScanLine(y - 1);
        const uchar* mid  = gray.constScanLine(y);
        const uchar* down = gray.constScanLine(y + 1);
        uchar* out = qDenoisingImage.scanLine(y);

        for (int x = 1; x < w - 1; x++) {
            if (filter == MEAN) {
                int sum = up[x-1] + up[x] + up[x+1]
                        + mid[x-1] + mid[x] + mid[x+1]
                        + down[x-1] + down[x] + down[x+1];
                out[x] = (sum + 4) / 9;                      // rounded average of the 3x3 block
            }
            else if (filter == MEDIAN) {
                int block[9] = { up[x-1], up[x], up[x+1],
                                 mid[x-1], mid[x], mid[x+1],
                                 down[x-1], down[x], down[x+1] };
                std::nth_element(block, block + 4, block + 9);
                out[x] = block[4];                           // middle of 9 values
            }
            else if (filter == GAUSSIAN) {
                int sum = up[x-1]   * gaussian3x3[0][0] + up[x]   * gaussian3x3[0][1] + up[x+1]   * gaussian3x3[0][2]
                        + mid[x-1]  * gaussian3x3[1][0] + mid[x]  * gaussian3x3[1][1] + mid[x+1]  * gaussian3x3[1][2]
                        + down[x-1] * gaussian3x3[2][0] + down[x] * gaussian3x3[2][1] + down[x+1] * gaussian3x3[2][2];
                out[x] = (sum + 8) / 16;                     // kernel weights add up to 16
            }
        }
    }
}
