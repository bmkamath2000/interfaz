#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    connect(ui->pushButtonLoadImage,SIGNAL(released()),this,SLOT(loadImage()));
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
       }
    }
}

void MainWindow::on_pushButtonSaveImage_clicked()
{
    clearResults();   // replace previous results instead of stacking them

    if(ui->radioButtonRGBtoHSL->isChecked()){
        transform->RGBtoHSL(image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = transform->getHSLImage();
        imageObject.save(imagePath);
        showImage(transform->getBlueImage(), "Blue");
        showImage(transform->getGreenImage(), "Green");
        showImage(transform->getRedImage(), "Red");
        showImage(imageObject, "HSL");
    }

    if (ui->radioButtonRGBtoHSV->isChecked()) {
        transform->RGBtoHSV(image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = transform->getHSVImage();
        imageObject.save(imagePath);
        showImage(transform->getBlueImage(), "Blue");
        showImage(transform->getGreenImage(), "Green");
        showImage(transform->getRedImage(), "Red");
        showImage(imageObject, "HSV");
    }

    if(ui->radioButtonMeanFilter->isChecked()){
        smooth->smoothing(MEAN,image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = smooth->getDenoisingImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonMedianFilter->isChecked()){
        smooth->smoothing(MEDIAN,image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = smooth->getDenoisingImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonGaussianFilter->isChecked()){
        smooth->smoothing(GAUSSIAN,image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = smooth->getDenoisingImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonEqualize->isChecked()){
        Histogram->equalizeHistogram(image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = Histogram->getEqualizedImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonIsodata->isChecked()){
        int thres = Histogram->calculateThreshold(ISODATA,image);
        printf("%d",thres);
        Histogram->thresholding(thres,image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = Histogram->getThresholdedImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonOtsu->isChecked()){
        int thres = Histogram->calculateThreshold(OTSU,image);
        Histogram->thresholding(thres,image);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = Histogram->getThresholdedImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonErosion->isChecked()){
        image = image.convertToFormat(QImage::Format_Grayscale8);
        morpho->thresholding(127,image);
        morpho->saveImage(morpho->getThresholdedImage());
        morpho->morpho(image,EROSION);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonDilate->isChecked()){
        image = image.convertToFormat(QImage::Format_Grayscale8);
        morpho->thresholding(127,image);
        morpho->saveImage(morpho->getThresholdedImage());
        morpho->morpho(image,DILATE);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonOpening->isChecked()){
        image = image.convertToFormat(QImage::Format_Grayscale8);
        morpho->thresholding(127,image);
        morpho->saveImage(morpho->getThresholdedImage());
        morpho->morpho(image,DILATE);
        morpho->saveImage(morpho->getMorpologicalImage());
        morpho->morpho(image,EROSION);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonClosing->isChecked()){
        image = image.convertToFormat(QImage::Format_Grayscale8);
        morpho->thresholding(127,image);
        morpho->saveImage(morpho->getThresholdedImage());
        morpho->morpho(image,EROSION);
        morpho->saveImage(morpho->getMorpologicalImage());
        morpho->morpho(image,DILATE);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonGSErosion->isChecked()){
        morpho->saveImage(image);
        morpho->morpho(image,GSEROSION);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonGSDilate->isChecked()){
        morpho->saveImage(image);
        morpho->morpho(image,GSDILATE);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonGSOpening->isChecked()){
        morpho->saveImage(image);
        morpho->morpho(image,GSDILATE);
        morpho->saveImage(morpho->getMorpologicalImage());
        morpho->morpho(image,GSEROSION);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonGSClosing->isChecked()){
        morpho->saveImage(image);
        morpho->morpho(image,GSEROSION);
        morpho->saveImage(morpho->getMorpologicalImage());
        morpho->morpho(image,GSDILATE);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
    }

    if(ui->radioButtonGSGradient->isChecked()){
        morpho->saveImage(image);
        morpho->morpho(image,GRADIENT);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        //pixmap = QPixmap::fromImage(imageObject);
        //ui->label_10->setPixmap(pixmap);
        showImage(imageObject);
    }
    if(ui->radioButtonGSTopHat->isChecked()){
        image = image.convertToFormat(QImage::Format_Grayscale8);
        morpho->saveImage(image);
        morpho->morpho(image,GSDILATE);
        morpho->saveImage(morpho->getMorpologicalImage());
        morpho->morpho(image,GSEROSION);
        morpho->saveImage(morpho->getMorpologicalImage());
        morpho->morpho(image,TOPHAT);
        QImage imageObject;
        QString imagePath = QFileDialog::getSaveFileName(this,tr("Save file"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));
        imageObject.load(imagePath);
        imageObject = morpho->getMorpologicalImage();
        imageObject.save(imagePath);
        showImage(imageObject);
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

// Shows the image inside the main window (results strip) instead of a new window
void MainWindow::showImage(QImage ImageToShow, const QString &caption){
    QWidget* cell = new QWidget();
    QVBoxLayout* cellLayout = new QVBoxLayout(cell);
    cellLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* imageLabel = new QLabel();
    imageLabel->setPixmap(QPixmap::fromImage(ImageToShow)
                          .scaled(280, 260, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
   ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonRGBtoHSV_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonMeanFilter_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonMedianFilter_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGaussianFilter_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

// Rebuilds the chart from scratch so repeated clicks (or alternating between
// the two histogram buttons) never accumulate series, axes or legend entries.
void MainWindow::showHistogramChart(const int* data, const QString& seriesName, const QString& title)
{
    // removeAllSeries() deletes the old series and their bar sets
    chart->removeAllSeries();
    // Axes created by createDefaultAxes() are not deleted by the chart: remove and delete them
    const auto oldAxes = chart->axes();
    for (QAbstractAxis* oldAxis : oldAxes) {
        chart->removeAxis(oldAxis);
        delete oldAxis;
    }

    QBarSet* set = new QBarSet(seriesName);
    QStringList categories;
    for (int i = 0; i < 256; i++) {
        categories.append(QString::number(i));   // note: "" + i was pointer arithmetic, not a string
        set->append(data[i]);
    }

    QBarSeries* series = new QBarSeries();
    series->setBarWidth(1.0);                    // bars touch: easier to read with 256 bins
    series->append(set);

    chart->addSeries(series);
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->createDefaultAxes();

    // Put the intensity labels on the x axis created by createDefaultAxes()
    const auto xAxes = chart->axes(Qt::Horizontal, series);
    if (!xAxes.isEmpty()) {
        QBarCategoryAxis* xAxis = qobject_cast<QBarCategoryAxis*>(xAxes.first());
        if (xAxis) {
            xAxis->append(categories);
            xAxis->setLabelsAngle(-90);
        }
    }

    chartView->setRenderHint(QPainter::Antialiasing);
    window.setCentralWidget(chartView);
    window.resize(420,300);
    window.show();
    window.raise();
}

void MainWindow::on_radioButtonHistogram_clicked()
{
    Histogram->calculateHistogram(image);
    showHistogramChart(Histogram->getHistogram(), "Intensity Pixels", "Histogram");
}

void MainWindow::on_radioButtonEquHistogram_clicked()
{
    Histogram->equalizeHistogram(image);
    showHistogramChart(Histogram->getEqualizedHistogram(), "Intensity of equalized pixels", "Equalized histogram");
}


void MainWindow::on_radioButtonEqualize_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonIsodata_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonOtsu_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonErosion_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonDilate_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonOpening_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonClosing_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGSGradient_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGSTopHat_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGSErosion_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGSDilate_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGSOpening_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}

void MainWindow::on_radioButtonGSClosing_clicked()
{
    ui->pushButtonSaveImage->setEnabled(true);
}
