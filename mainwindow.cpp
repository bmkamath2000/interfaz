#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    transform = new ColorTransformation();
    smooth = new noise();
    Histogram = new histogram();
    morpho = new morphological();

    // Results strip: lives in the empty area below the controls of this window
    resultArea = new QScrollArea(ui->centralWidget);
    resultArea->setGeometry(20, 320, 780, 310);
    resultArea->setWidgetResizable(true);
    resultArea->setFrameShape(QFrame::StyledPanel);
    QWidget* resultContainer = new QWidget();
    resultLayout = new QHBoxLayout(resultContainer);
    resultLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    resultArea->setWidget(resultContainer);

    deactivateFunctions();
    // (Load button is wired automatically to on_pushButtonLoadImage_clicked; the old
    //  connect() to a non-existent loadImage() slot only printed a warning.)
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonLoadImage_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,tr("Choose file"),"",tr("images (*.png *.jpg *.jpeg)"));
    if(QString::compare(filename,QString()) != 0){
       bool valid = image.load(filename);
       if(valid){
           QImage copy = image;
           copy = copy.scaled(ui->labelImage->width(),ui->labelImage->height(), Qt::KeepAspectRatio);
           ui->route->setText(filename);
           ui->labelImage->setPixmap(QPixmap::fromImage(copy));
           activateFunctions();
           clearResults();                                // previous results belong to the old image
           currentResult = QImage();
           ui->pushButtonSaveImage->setEnabled(false);
       }
    }
}

// Qt pads every image row to a multiple of 4 bytes. The morphological code indexes pixels
// as x + y * width, which is only valid when the width is a multiple of 4, so pad the
// image on the right (repeating the last column) and crop the result afterwards.
static QImage padForLinearAccess(const QImage &gray)
{
    const int w = gray.width();
    const int w4 = (w + 3) / 4 * 4;
    if (w4 == w) return gray;

    QImage out(w4, gray.height(), QImage::Format_Grayscale8);
    for (int y = 0; y < gray.height(); y++) {
        const uchar* src = gray.constScanLine(y);
        uchar* dst = out.scanLine(y);
        memcpy(dst, src, w);
        for (int x = w; x < w4; x++) dst[x] = src[w - 1];
    }
    return out;
}

// Runs the operation selected in the radio buttons on the loaded image, shows the
// result(s) in the results strip and remembers the main result for "Save image".
void MainWindow::runSelectedOperation()
{
    if (image.isNull()) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QList<QPair<QString, QImage>> outputs;   // everything shown in the results strip
    QImage result;                           // what "Save image" will write
    const int w = image.width();
    const int h = image.height();

    if (ui->radioButtonRGBtoHSL->isChecked()) {
        transform->RGBtoHSL(image);
        result = transform->getHSLImage();
        outputs << qMakePair(QString("Red"), transform->getRedImage())
                << qMakePair(QString("Green"), transform->getGreenImage())
                << qMakePair(QString("Blue"), transform->getBlueImage())
                << qMakePair(QString("HSL"), result);
    }
    else if (ui->radioButtonRGBtoHSV->isChecked()) {
        transform->RGBtoHSV(image);
        result = transform->getHSVImage();
        outputs << qMakePair(QString("Red"), transform->getRedImage())
                << qMakePair(QString("Green"), transform->getGreenImage())
                << qMakePair(QString("Blue"), transform->getBlueImage())
                << qMakePair(QString("HSV"), result);
    }
    else if (ui->radioButtonMeanFilter->isChecked()) {
        smooth->smoothing(MEAN, image);
        result = smooth->getDenoisingImage();
        outputs << qMakePair(QString("Mean filter"), result);
    }
    else if (ui->radioButtonMedianFilter->isChecked()) {
        smooth->smoothing(MEDIAN, image);
        result = smooth->getDenoisingImage();
        outputs << qMakePair(QString("Median filter"), result);
    }
    else if (ui->radioButtonGaussianFilter->isChecked()) {
        smooth->smoothing(GAUSSIAN, image);
        result = smooth->getDenoisingImage();
        outputs << qMakePair(QString("Gaussian filter"), result);
    }
    else if (ui->radioButtonEqualize->isChecked()) {
        Histogram->equalizeHistogram(image);
        result = Histogram->getEqualizedImage();
        outputs << qMakePair(QString("Equalized"), result);
    }
    else if (ui->radioButtonIsodata->isChecked()) {
        int thres = Histogram->calculateThreshold(ISODATA, image);
        Histogram->thresholding(thres, image);
        result = Histogram->getThresholdedImage();
        outputs << qMakePair(QString("Isodata (threshold %1)").arg(thres), result);
    }
    else if (ui->radioButtonOtsu->isChecked()) {
        int thres = Histogram->calculateThreshold(OTSU, image);
        Histogram->thresholding(thres, image);
        result = Histogram->getThresholdedImage();
        outputs << qMakePair(QString("Otsu (threshold %1)").arg(thres), result);
    }
    else {
        // Morphological operations: all work on a local grayscale copy so the loaded
        // image is never modified.
        const QImage g = padForLinearAccess(image.convertToFormat(QImage::Format_Grayscale8));
        QString caption;

        // binary images: threshold at 127 first
        auto binaryPrepare = [&]() {
            morpho->thresholding(127, g);
            morpho->saveImage(morpho->getThresholdedImage());
        };

        if (ui->radioButtonErosion->isChecked()) {
            caption = "Binary erosion";
            binaryPrepare();
            morpho->morpho(g, EROSION);
        }
        else if (ui->radioButtonDilate->isChecked()) {
            caption = "Binary dilation";
            binaryPrepare();
            morpho->morpho(g, DILATE);
        }
        else if (ui->radioButtonOpening->isChecked()) {
            caption = "Binary opening";
            binaryPrepare();
            morpho->morpho(g, DILATE);
            morpho->saveImage(morpho->getMorpologicalImage());
            morpho->morpho(g, EROSION);
        }
        else if (ui->radioButtonClosing->isChecked()) {
            caption = "Binary closing";
            binaryPrepare();
            morpho->morpho(g, EROSION);
            morpho->saveImage(morpho->getMorpologicalImage());
            morpho->morpho(g, DILATE);
        }
        else if (ui->radioButtonGSErosion->isChecked()) {
            caption = "Grayscale erosion";
            morpho->saveImage(g);
            morpho->morpho(g, GSEROSION);
        }
        else if (ui->radioButtonGSDilate->isChecked()) {
            caption = "Grayscale dilation";
            morpho->saveImage(g);
            morpho->morpho(g, GSDILATE);
        }
        else if (ui->radioButtonGSOpening->isChecked()) {
            caption = "Grayscale opening";
            morpho->saveImage(g);
            morpho->morpho(g, GSDILATE);
            morpho->saveImage(morpho->getMorpologicalImage());
            morpho->morpho(g, GSEROSION);
        }
        else if (ui->radioButtonGSClosing->isChecked()) {
            caption = "Grayscale closing";
            morpho->saveImage(g);
            morpho->morpho(g, GSEROSION);
            morpho->saveImage(morpho->getMorpologicalImage());
            morpho->morpho(g, GSDILATE);
        }
        else if (ui->radioButtonGSGradient->isChecked()) {
            caption = "Gradient";
            morpho->saveImage(g);
            morpho->morpho(g, GRADIENT);
        }
        else if (ui->radioButtonGSTopHat->isChecked()) {
            caption = "Top hat";
            morpho->saveImage(g);
            morpho->morpho(g, GSDILATE);
            morpho->saveImage(morpho->getMorpologicalImage());
            morpho->morpho(g, GSEROSION);
            morpho->saveImage(morpho->getMorpologicalImage());
            morpho->morpho(g, TOPHAT);
        }

        if (!caption.isEmpty()) {
            result = morpho->getMorpologicalImage().copy(0, 0, w, h);   // drop the padding columns
            outputs << qMakePair(caption, result);
        }
    }

    QApplication::restoreOverrideCursor();

    currentResult = result;
    ui->pushButtonSaveImage->setEnabled(!result.isNull());
    displayResults(outputs);
}

// "Save image" now only writes the result that is already on screen.
void MainWindow::on_pushButtonSaveImage_clicked()
{
    if (currentResult.isNull()) return;

    QString imagePath = QFileDialog::getSaveFileName(this, tr("Save file"), "",
                                                     tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
    if (imagePath.isEmpty()) return;                       // dialog cancelled
    if (QFileInfo(imagePath).suffix().isEmpty()) imagePath += ".png";

    if (!currentResult.save(imagePath)) {
        QMessageBox::warning(this, tr("Save image"), tr("Could not save the image to:\n%1").arg(imagePath));
    }
}

void MainWindow::deactivateFunctions(){
    ui->radioButtonRGBtoHSL->setEnabled(false);
    ui->radioButtonRGBtoHSV->setEnabled(false);
    ui->radioButtonMeanFilter->setEnabled(false);
    ui->radioButtonMedianFilter->setEnabled(false);
    ui->radioButtonHistogram->setEnabled(false);
    ui->radioButtonGaussianFilter->setEnabled(false);
    ui->pushButtonSaveImage->setEnabled(false);
    ui->radioButtonEqualize->setEnabled(false);
    ui->radioButtonEquHistogram->setEnabled(false);
    ui->radioButtonIsodata->setEnabled(false);
    ui->radioButtonOtsu->setEnabled(false);
    ui->radioButtonErosion->setEnabled(false);
    ui->radioButtonDilate->setEnabled(false);
    ui->radioButtonOpening->setEnabled(false);
    ui->radioButtonClosing->setEnabled(false);

    ui->radioButtonGSErosion->setEnabled(false);
    ui->radioButtonGSDilate->setEnabled(false);
    ui->radioButtonGSOpening->setEnabled(false);
    ui->radioButtonGSClosing->setEnabled(false);
    ui->radioButtonGSGradient->setEnabled(false);
    ui->radioButtonGSTopHat->setEnabled(false);
}

void MainWindow::activateFunctions(){
    ui->radioButtonRGBtoHSL->setEnabled(true);
    ui->radioButtonRGBtoHSV->setEnabled(true);
    ui->radioButtonMeanFilter->setEnabled(true);
    ui->radioButtonMedianFilter->setEnabled(true);
    ui->radioButtonGaussianFilter->setEnabled(true);
    ui->radioButtonHistogram->setEnabled(true);
    ui->radioButtonEquHistogram->setEnabled(true);
    ui->radioButtonEqualize->setEnabled(true);
    ui->radioButtonIsodata->setEnabled(true);
    ui->radioButtonOtsu->setEnabled(true);
    ui->radioButtonErosion->setEnabled(true);
    ui->radioButtonDilate->setEnabled(true);
    ui->radioButtonOpening->setEnabled(true);
    ui->radioButtonClosing->setEnabled(true);

    ui->radioButtonGSErosion->setEnabled(true);
    ui->radioButtonGSDilate->setEnabled(true);
    ui->radioButtonGSOpening->setEnabled(true);
    ui->radioButtonGSClosing->setEnabled(true);
    ui->radioButtonGSGradient->setEnabled(true);
    ui->radioButtonGSTopHat->setEnabled(true);
}

void MainWindow::clearResults(){
    while (QLayoutItem* child = resultLayout->takeAt(0)) {
        delete child->widget();
        delete child;
    }
}

// Replaces the contents of the results strip with the given (caption, image) pairs,
// sized so they fit the strip without scrolling where possible.
void MainWindow::displayResults(const QList<QPair<QString, QImage>> &outputs){
    clearResults();
    if (outputs.isEmpty()) return;

    const QSize area = resultArea->viewport()->size();
    const int n = outputs.size();
    const int tileW = qMax(120, (area.width() - 20) / n - 10);
    const int tileH = qMax(120, area.height() - 50);        // leave room for the caption
    for (const auto &item : outputs) {
        showImage(item.second, item.first, QSize(tileW, tileH));
    }
}

// Adds one image (with optional caption) to the results strip in the main window
void MainWindow::showImage(QImage ImageToShow, const QString &caption, const QSize &maxSize){
    QWidget* cell = new QWidget();
    QVBoxLayout* cellLayout = new QVBoxLayout(cell);
    cellLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* imageLabel = new QLabel();
    imageLabel->setPixmap(QPixmap::fromImage(ImageToShow)
                          .scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    cellLayout->addWidget(imageLabel);

    if (!caption.isEmpty()) {
        QLabel* captionLabel = new QLabel(caption);
        captionLabel->setAlignment(Qt::AlignHCenter);
        cellLayout->addWidget(captionLabel);
    }
    resultLayout->addWidget(cell);
}

void MainWindow::on_radioButtonRGBtoHSL_clicked()
{
   runSelectedOperation();
}

void MainWindow::on_radioButtonRGBtoHSV_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonMeanFilter_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonMedianFilter_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGaussianFilter_clicked()
{
    runSelectedOperation();
}

// Rebuilds the chart from scratch so repeated clicks never stack series/axes/legends
void MainWindow::showHistogramChart(const QString &title, const QString &setName, const int *data)
{
    chart->removeAllSeries();
    const QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* a : oldAxes) {
        chart->removeAxis(a);
        delete a;
    }

    QBarSet* set = new QBarSet(setName);
    QStringList categories;
    int maxValue = 0;
    for (int i = 0; i < 256; i++) {
        set->append(data[i]);
        categories.append(QString::number(i));
        if (data[i] > maxValue) maxValue = data[i];
    }

    QBarSeries* series = new QBarSeries();
    series->setBarWidth(1.0);
    series->append(set);
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::NoAnimation);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsAngle(-90);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, maxValue);
    axisY->applyNiceNumbers();
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView->setRenderHint(QPainter::Antialiasing);
    window.setCentralWidget(chartView);
    window.resize(900, 400);
    window.show();
    window.raise();
}

void MainWindow::on_radioButtonHistogram_clicked()
{
    clearResults();                                   // chart only: nothing to save
    currentResult = QImage();
    ui->pushButtonSaveImage->setEnabled(false);
    Histogram->calculateHistogram(image);
    showHistogramChart("Histogram", "Intensity Pixels", Histogram->getHistogram());
}

void MainWindow::on_radioButtonEquHistogram_clicked()
{
    clearResults();                                   // chart only: nothing to save
    currentResult = QImage();
    ui->pushButtonSaveImage->setEnabled(false);
    Histogram->equalizeHistogram(image);
    showHistogramChart("Equalized histogram", "Intensity of equalized pixels", Histogram->getEqualizedHistogram());
}


void MainWindow::on_radioButtonEqualize_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonIsodata_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonOtsu_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonErosion_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonDilate_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonOpening_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonClosing_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGSGradient_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGSTopHat_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGSErosion_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGSDilate_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGSOpening_clicked()
{
    runSelectedOperation();
}

void MainWindow::on_radioButtonGSClosing_clicked()
{
    runSelectedOperation();
}
