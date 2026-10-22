#include "histogram.h"
#include "QImageWriter"
histogram::histogram()
{

}

void histogram::calculateHistogram(QImage image){
    // Keep the grayscale copy as a member: the old code stored a pointer into a local
    // image that was freed on return, and equalizeHistogram() then read through it.
    qOriginalImage = image.convertToFormat(QImage::Format_Grayscale8);
    memset(histogramArray, 0, sizeof(histogramArray));
    for (int y = 0; y < qOriginalImage.height(); y++) {
        const uchar* line = qOriginalImage.constScanLine(y);   // rows are padded to 4 bytes
        for (int x = 0; x < qOriginalImage.width(); x++) {
            histogramArray[line[x]]++;
        }
    }
}

void histogram::calculateAccumulatedFrequency(){
    accumulatedFrequency[0] = histogramArray[0];

    for (int frequency = 1; frequency < 256; frequency++) {
        accumulatedFrequency[frequency] = histogramArray[frequency] +  accumulatedFrequency[frequency-1];
    }
}

void histogram::equalizeHistogram(QImage image){

    qEqualizedImage = QImage(image.width(),image.height(),QImage::Format_Grayscale8);
    memset(equalizedHistogram, 0, sizeof(equalizedHistogram));
    calculateHistogram(image);
    calculateAccumulatedFrequency();

    float scale = 255.0f/accumulatedFrequency[255];
    for (int value = 0; value < 256; ++value) {
        newValues[value] = (scale * accumulatedFrequency[value]);
    }

    for (int y = 0; y < qOriginalImage.height(); y++) {
        const uchar* src = qOriginalImage.constScanLine(y);
        uchar* dst = qEqualizedImage.scanLine(y);
        for (int x = 0; x < qOriginalImage.width(); x++) {
            int value = newValues[src[x]];
            dst[x] = value;
            equalizedHistogram[value]++;
        }
    }
}

int histogram::calculateThreshold(ThresMethod thres, QImage image){
    int threshold = 0;

    if(thres == ISODATA){
        calculateHistogram(image);
        calculateAccumulatedFrequency();
        int oldT = 0, newT = 128;

        while(oldT != newT){
            oldT = newT;
            float meanF = 0.0f;
            float meanB = 0.0f;

            for (int f = 0; f < newT; f++) {
                meanF += f * histogramArray[f];
            }

            // pixels below / at-or-above the threshold; stop if either side is empty
            // (the old code divided by zero and indexed [-1] when newT reached 0)
            int countF = newT > 0 ? accumulatedFrequency[newT-1] : 0;
            int countB = accumulatedFrequency[255] - countF;
            if (countF == 0 || countB == 0) break;

            meanF = meanF/countF;

            for (int b = newT; b < 256; b++) {
                meanB += b * histogramArray[b];
            }

            meanB = meanB / countB;

            newT = (meanF + meanB) / 2;
        }
        return newT;
    }

    else if(thres==OTSU){
        calculateHistogram(image);
        calculateAccumulatedFrequency();

        int imageSize = image.width() * image.height();
        int     sum = 0;
        int     q1 = 0; // class 1
        int     q2 = 0; // class 2
        float   meanF = 0.0;
        float   max = 0;  // maximun -- threshold

        for (int i = 0; i < 256; i++){
          sum += i * ((int)histogramArray[i]);
        }

        for (int i = 0 ; i <256 ; i++) {

            q1 += histogramArray[i];
            q2 = imageSize - q1;

            meanF += (float) (i * ((int)histogramArray[i]));
            if (q1 == 0 || q2 == 0) continue;
            float m1 = meanF / q1;
            float m2 = (sum - meanF) / q2;

            float temp = (float) q1 * (float) q2 * (m1 - m2) * (m1 - m2);

            // Update the threshold if necessary
            if (temp > max) {
              max = temp;
              threshold = i;
            }
        }
        return threshold;
    }
    return 0;
}

void histogram::thresholding(int threshold,QImage image){
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    qThresholdedImage = QImage(gray.width(), gray.height(), QImage::Format_Grayscale8);

    for (int y = 0; y < gray.height(); y++) {
        const uchar* src = gray.constScanLine(y);
        uchar* dst = qThresholdedImage.scanLine(y);
        for (int x = 0; x < gray.width(); x++) {
            dst[x] = src[x] < threshold ? 0 : 255;
        }
    }
}
