#include <iostream>
#include "EdgeDetection.h"
#include "opencv2/opencv.hpp"
#include "../stream/CameraStream.h"
#include <math.h>

using namespace std;
using namespace cv;

///Coordonnées du point permettant la calibration pour la mask sur le niveau de gris
int StartingPointX = 0 ;
int StartingPointY = 0;
int points[4][2];

EdgeDetection::EdgeDetection(Mat img, bool ini){

    if(ini){
        points[0][0] = 0;
        points[0][1] = 0;

        points[1][0] = img.cols;
        points[1][1] = 0;

        points[2][0] = img.cols;
        points[2][1] = img.rows;

        points[3][0] = 0;
        points[3][1] = img.rows;
    }
}

///Fonction permettant la calibration de la couleur
Mat EdgeDetection::colorCalibration(Mat img){

    Mat mask = buildBasicMask(img);

    ///On manipule le mask afin de ne récupérer que les 4 coins
    Mat maskTemp = mask.clone();
    cv::floodFill(maskTemp, cv::Point(StartingPointX, StartingPointY), CV_RGB(0, 0, 0));

    ///Inversion du mask
    maskTemp = ~maskTemp;
    mask = maskTemp & mask;
    cv::floodFill(mask, cv::Point(0,0), CV_RGB(255, 255, 255));

    ///On enlève les parasites
    Mat kernel;
    kernel = getStructuringElement(2, Size(7,7), Point(2,2));
    dilate(mask, mask, kernel);
    erode(mask, mask, kernel);

    circle(mask, Point2i(StartingPointX,StartingPointY), 5, Scalar(150,150,150));

    ///On retourne le mask
    return mask;

}

/// cree un le masque qui sert de point de depart pour tout les autres
Mat EdgeDetection::buildBasicMask(Mat img){
    Mat imgGrey;
    cvtColor(img, imgGrey, COLOR_RGB2GRAY);

    ///On initialise le point de calibration au milieu de l'écran
    if(StartingPointX == 0 && StartingPointY == 0){
        StartingPointX = imgGrey.cols / 2 ;
        StartingPointY = imgGrey.rows / 2 ;
    }


    int nivMax = 0;
    int nivGrey, tempx, tempy;

    ///On récupère le niveau de gris max du voisinage du pixel du milieu
    ///on calcule l'histogramme des niveaux de gris
    vector<int> histo = vector<int>(256);
    for(int i = -10; i < 10; i++){
        for(int j = -10; j < 10; j++) {
            if (0 <= StartingPointY + i && StartingPointY + i < img.rows && 0 <= StartingPointX + j && StartingPointX + j < img.cols){
                histo[(int) imgGrey.at<uchar>(StartingPointY + i, StartingPointX + j)] ++;
                nivGrey = (int) imgGrey.at<uchar>(StartingPointY + i, StartingPointX + j);
                if (nivGrey > nivMax) {
                    nivMax = nivGrey;
                    tempx = StartingPointX + j;
                    tempy = StartingPointY + i;
                }
            }
        }
    }
    /// on recupere le niveau de gris le plus present : correspond a la couleur blanche de la feuille
    int max = 0;
    int maxBlack = 0;
    int maxWhite = 0;
    for(int i=0 ; i< 128 ; i++){
        if(histo[i]>max){
            maxBlack = i;
            max = histo[i];
        }
    }
    max=0;
    for(int i=128 ; i< 256 ; i++){
        if(histo[i]> max){
            maxWhite = i;
            max = histo[i];
        }
    }

    StartingPointX = tempx;
    StartingPointY = tempy;

    ///On créé le mask en fonction du niveau de gris précédent
    Mat mask;
    inRange(imgGrey, maxWhite - 60, 255, mask);
    return mask;
}

vector<Point2i> EdgeDetection::getCorner(Mat img) {
    vector<Point2i> coordCorner;

    /// recréation d'un mask quivabien
    Mat newMask = buildBasicMask(img);
    int rows = newMask.rows;
    int cols = newMask.cols;

    Mat maskTemp = newMask.clone();
    cv::floodFill(maskTemp, cv::Point(StartingPointX, StartingPointY), CV_RGB(0, 0, 0));
    maskTemp = ~maskTemp;
    newMask = maskTemp & newMask;
    maskTemp = newMask.clone();
    cv::floodFill(maskTemp, cv::Point(0,0), CV_RGB(255, 255, 255));
    maskTemp = ~maskTemp;
    newMask= newMask | maskTemp;
    Mat kernel1;
    kernel1 = getStructuringElement(1, Size(7,7), Point(2,2));
    dilate(newMask, newMask, kernel1);
    erode(newMask, newMask, kernel1);

    int trackingSize = 15;
    int imax, jmax, noirMax,nbNoirs, kernel =7;

    for (int ip = 0; ip < 4; ip++) {
        imax = 0;
        jmax = 0;
        noirMax = 0;
        for (int i = points[ip][1] - trackingSize; i < points[ip][1] + trackingSize; i++) {
            for (int j = points[ip][0] - trackingSize; j < points[ip][0] + trackingSize; j++) {
                if (i >= 0 && i < rows && j >= 0 && j < cols && newMask.at<uchar>(i, j) == 255) {
                    nbNoirs = 0;
                    ///calcul du nb de noirs dans le voisinage
                    for (int ii = i - kernel; ii < i + kernel; ii++) {
                        for (int jj = j - kernel; jj < j + kernel; jj++) {
                            if (ii >= 0 && ii < rows && jj >= 0 && jj < cols &&
                                newMask.at<uchar>(ii, jj) == 0) {
                                nbNoirs++;
                            }
                        }
                    }

                    if (nbNoirs > kernel*kernel*4*3/5 && nbNoirs > noirMax) {
                        noirMax = nbNoirs;
                        imax = i;
                        jmax = j;
                    }
                }
            }
        }
        coordCorner.push_back(Point(jmax, imax));
    }
    coordCorner = pointsVerification(coordCorner);
    bool isOk = true;
    bool isOk2 = true;
    for(int i = 0; i < 4; i++){
        isOk &= coordCorner[i].x != 0 && coordCorner[i].y != 0 ;
        for(int j = i+1 ; j<4 ; j++){
            isOk2&= (coordCorner[i].x < coordCorner[j].x - 10 || coordCorner[i].x > coordCorner[j].x +10
                    || coordCorner[i].y < coordCorner[j].y - 10 || coordCorner[i].y > coordCorner[j].y +10) ;
        }
    }
    isOk&=isOk2;

    if(isOk){
        //cout << "VOISINAGE" << endl;

    }
    else{
        //cout << "PARCOURS ENTIER" << endl;
        coordCorner = getCornerMinMax(newMask);
    }

    for(int i = 0; i < 4; i++){
        circle(newMask, Point(coordCorner[i].x,coordCorner[i].y),i*20+8,Scalar(255,0,0));
        points[i][0] = coordCorner[i].x;
        points[i][1] = coordCorner[i].y;
    }
//    namedWindow("newMask",WINDOW_AUTOSIZE);
//    imshow("newMask",newMask);

    StartingPointX = (coordCorner[0].x + coordCorner[1].x + coordCorner[2].x + coordCorner[3].x)/4 ;
    StartingPointY = (coordCorner[0].y + coordCorner[1].y + coordCorner[2].y + coordCorner[3].y)/4 ;

    return coordCorner ;
}

vector<Point2i> EdgeDetection::getCornerMinMax(Mat mask) {
    vector<Point2i> coordCorner;

    /// recréation d'un mask quivabien
    Mat newMask = mask;
    int rows = newMask.rows;
    int cols = newMask.cols;

    int minxy = cols + rows;
    int maxx_y = -cols - rows;
    int maxxy = -cols - rows;
    int maxy_x = -cols - rows;
    int  xminxy, yminxy, xmaxx_y, ymaxx_y, xmaxxy, ymaxxy, xmaxy_x, ymaxy_x;
    /*int minxy = cols+rows, xminxy, yminxy;
    int maxx_y = -cols-rows, xmaxx_y, ymaxx_y ;
    int maxxy =-cols-rows, xmaxxy, ymaxxy;
    int maxy_x = -cols-rows, xmaxy_x, ymaxy_x;*/

    for (int i = 1; i < rows; i++) {
        for (int j = 1; j < cols; j++) {
            if (newMask.at<uchar>(i, j) == 255) {
                if(i+j < minxy){
                    minxy=i+j;
                    xminxy=j;
                    yminxy=i;
                }if(j-i>maxx_y){
                    maxx_y=j-i;
                    xmaxx_y =j;
                    ymaxx_y =i;
                }if(i+j > maxxy){
                    maxxy=i+j;
                    xmaxxy = j;
                    ymaxxy=i;
                }if(i-j>maxy_x){
                    maxy_x = i-j;
                    xmaxy_x=j;
                    ymaxy_x=i;
                }
            }
        }
    }

    coordCorner.push_back(Point(xminxy,yminxy));
    coordCorner.push_back(Point(xmaxx_y,ymaxx_y));
    coordCorner.push_back(Point(xmaxxy,ymaxxy));
    coordCorner.push_back(Point(xmaxy_x,ymaxy_x));
    //printf("%d %d %d %d %d %d \n", minxy, maxx_y, maxxy, maxy_x, cols, rows);
    //printf("%d %d, %d %d, %d %d, %d %d \n", xminxy, yminxy, xmaxx_y, ymaxx_y, xmaxxy, ymaxxy, xmaxy_x, ymaxy_x);
    //printf("%d %d \n", coordCorner[0].x, coordCorner[0].y);
    return coordCorner ;

}

///Fonctions permettant de détecter le départ et l'arrivé de la boule
vector<Point2i> EdgeDetection::startEndDetection(Mat img) {

    ///Initialisation des variables
    Mat mask = colorCalibration(img);
    std::vector<KeyPoint> point;
    std::vector<KeyPoint> point_fin;
    vector<Point2i> coordPoint;

    ///Paramètre pour la détection des composantes connexes
    SimpleBlobDetector::Params params;
    params.minThreshold = 0;
    params.maxThreshold = 100;
    params.filterByArea = true;
    params.minArea = 100;
    params.maxArea = 10000;
    params.filterByCircularity = true;
    //pour un cercle, qui definit le début (normalement = 1)
    params.minCircularity = 0.855;
    params.maxCircularity = 1.5;

    params.filterByConvexity = false;
    params.filterByInertia = false;

    Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
    detector->detect(mask, point);

    //si on detecte 2 circle, on classe le debut et la fin du jeu coomme avant
    if (point.size() == 2) {
        for (int i = 0; i < 2; i++) {
            coordPoint.push_back(point[i].pt);
        }
    }

    //sinon, le debut est le cercle (rangé à l'emplacement 0)
    else if (point.size() == 1) {
        coordPoint.push_back(point[0].pt);

        //pour un carre 0.785, definit  donc la fin
        params.minCircularity = 0.55;
        params.maxCircularity = 0.855;

        detector = cv::SimpleBlobDetector::create(params);
        detector->detect(mask, point_fin);

        //et la fin definit par un carré
        if (point_fin.size() == 1) {
            coordPoint.push_back(point_fin[0].pt);
        }
    }
    return coordPoint;
}


vector<Point2i> EdgeDetection::pointsVerification(vector<Point2i> coord){
    vector<Point2i> temp;
    int valTemp1 = 10000, valTemp2, indice;

    /// On cherche min x+y
    for(int i = 0; i < 4; i++){
        valTemp2 = coord[i].x+coord[i].y;
        if(valTemp2 < valTemp1) {
            valTemp1 = valTemp2;
            indice = i;
        }
    }
    temp.push_back(coord[indice]);

    /// On cherche max x-y
    valTemp1 = -10000;
    for(int i = 0; i < 4; i++){
        valTemp2 = coord[i].x-coord[i].y;
        if(valTemp2 > valTemp1) {
            valTemp1 = valTemp2;
            indice = i;
        }
    }
    temp.push_back(coord[indice]);

    ///On cherche max x+y
    valTemp1 = 0;
    for(int i = 0; i < 4; i++){
        valTemp2 = coord[i].x+coord[i].y;
        if(valTemp2 > valTemp1) {
            valTemp1 = valTemp2;
            indice = i;
        }
    }
    temp.push_back(coord[indice]);

    ///On cherche max y-x
    valTemp1 = -10000;
    for(int i = 0; i < 4; i++){
        valTemp2 = coord[i].y-coord[i].x;
        if(valTemp2 > valTemp1) {
            valTemp1 = valTemp2;
            indice = i;
        }
    }
    temp.push_back(coord[indice]);

    return temp;
}

///Fonction permettant la détection des lignes
vector<vector<Point2i>> EdgeDetection::wallsDetection(Mat img, vector<Point2i> coordCorner, vector<Point2i> coordStartEnd){

    Mat newMask = buildBasicMask(img);
    Mat maskTemp = newMask.clone();
    cv::floodFill(maskTemp, cv::Point(StartingPointX, StartingPointY), CV_RGB(0, 0, 0));
    maskTemp = ~maskTemp;
    newMask = maskTemp & newMask;
    line(newMask, coordCorner[0],coordCorner[1],Scalar(255,255,255),3);
    line(newMask, coordCorner[1],coordCorner[2],Scalar(255,255,255),3);
    line(newMask, coordCorner[2],coordCorner[3],Scalar(255,255,255),3);
    line(newMask, coordCorner[3],coordCorner[0],Scalar(255,255,255),3);
    maskTemp = newMask.clone();
    cv::floodFill(maskTemp, cv::Point(0,0), CV_RGB(255, 255, 255));
    maskTemp = ~maskTemp;

    if(coordStartEnd.size() == 2){
        floodFill(maskTemp, coordStartEnd[0],Scalar(0,0,0));
        floodFill(maskTemp,coordStartEnd[1],Scalar(0,0,0));
    }

//    namedWindow("maskMurs",WINDOW_AUTOSIZE);
//    imshow("maskMurs", maskTemp);

    /// tableau de couples de points
    vector<Vec4i> vectLinesTemp;
    vectLinesTemp = linesDetection(maskTemp, 25,25,5);

    vector<vector<Point2i>> vectLines;
    vector<Point2i> lineTemp ;
    lineTemp.push_back(coordCorner[0]);
    lineTemp.push_back(coordCorner[1]);
    vectLines.push_back(lineTemp);
    lineTemp.clear();
    lineTemp.push_back(coordCorner[1]);
    lineTemp.push_back(coordCorner[2]);
    vectLines.push_back(lineTemp);
    lineTemp.clear();
    lineTemp.push_back(coordCorner[2]);
    lineTemp.push_back(coordCorner[3]);
    vectLines.push_back(lineTemp);
    lineTemp.clear();
    lineTemp.push_back(coordCorner[3]);
    lineTemp.push_back(coordCorner[0]);
    vectLines.push_back(lineTemp);
    lineTemp.clear();

    for(Vec4i line: vectLinesTemp){
        cv::line(maskTemp, Point(line[0],line[1]),Point(line[2],line[3]),Scalar(0,0,0),3, LINE_AA);
        lineTemp.push_back(Point(line[0],line[1]));
        lineTemp.push_back(Point(line[2],line[3]));
        vectLines.push_back(lineTemp);
        lineTemp.clear();
    }
    vectLinesTemp.clear();

    int nbWhite = 0;
    for(int i = 0 ; i<maskTemp.cols ; i++){
        for(int j = 0 ; j<maskTemp.rows ; j++){
            if((int)maskTemp.at<uchar>(j,i) == 255)
                nbWhite ++ ;
        }
    }

//    namedWindow("maskMursApres1erePasse",WINDOW_AUTOSIZE);
//    imshow("maskMursApres1erePasse", maskTemp);

    if(nbWhite > 0){
        vectLinesTemp = linesDetection(maskTemp,20,10,3);
        for(Vec4i line: vectLinesTemp){
            cv::line(maskTemp, Point(line[0],line[1]),Point(line[2],line[3]),Scalar(0,0,0),3, LINE_AA);
            lineTemp.push_back(Point(line[0],line[1]));
            lineTemp.push_back(Point(line[2],line[3]));
            vectLines.push_back(lineTemp);
            lineTemp.clear();
        }
    }

//    namedWindow("maskMursApres2emePasse",WINDOW_AUTOSIZE);
//    imshow("maskMursApres2emePasse", maskTemp);

    return(filterDouble(vectLines,15));
}

vector<Vec4i> EdgeDetection::linesDetection(Mat mask, int thresh, int minLength, int maxGap ){
    /// detection des lignes dans le vect lines
    /// vecteur dans lequel sont stockées les lignes
    ///     lignes stockées sous la forme (x1,y1,x2,y2)
    /// houghLinesP(imgsource,
    /// vectdest,
    /// distance resolution en pixels
    /// angle resolution en rad
    /// seuil :The minimum number of intersections to “detect” a line
    /// longueur min d'une ligne détectée
    /// max ecart entre pixels de la ligne)
    vector<Vec4i> linesVect ;
    HoughLinesP(mask, linesVect, 1, CV_PI/180, thresh, minLength, maxGap);
    return linesVect ;
}

vector<vector<Point2i>> EdgeDetection::filterDouble(vector<vector<Point2i>> vectLines, int thresh){
    vector<vector<Point2i>> linesFilter = vectLines ;

    int suppressed = 0;
    int incr = 0 ;
    for(vector<Point2i> line : vectLines) {
        bool isGood = true;
        for (vector<Point2i> goodLine : linesFilter) {
            if (line[0].x != goodLine[0].x && line[0].y != goodLine[0].y && line[1].x != goodLine[1].x &&
                line[1].y != goodLine[1].y) {
                int minX = min(goodLine[0].x, goodLine[1].x) - thresh;
                int maxX = max(goodLine[0].x, goodLine[1].x) + thresh;
                int minY = min(goodLine[0].y, goodLine[1].y) - thresh;
                int maxY = max(goodLine[0].y, goodLine[1].y) + thresh;
                isGood &= (line[0].x > maxX || line[0].x < minX || line[0].y > maxY || line[0].y < minY) ||
                          (line[1].x > maxX || line[1].x < minX || line[1].y > maxY || line[1].y < minY);
            }
        }
        if (!isGood) {
            linesFilter.erase(linesFilter.begin()+ (incr-suppressed));
            suppressed ++;
        }
        incr++ ;
    }

    return linesFilter ;
}
/// fonction utilisée pour trier les points
bool sortByY(Point p1, Point p2){
    return p1.y>p2.y ;
}

///Fonction permettant de savoir s'il y a une inversion dans le plan
bool EdgeDetection::isReversed(vector<Point2i> &corners){
    vector<Point2i> cop;
    for(int i = 0; i < corners.size(); i++){
        cop.push_back(corners[i]);
    }
    sort(cop.begin(), cop.end(), sortByY);
    return fabs(cop[0].x - cop[1].x) < fabs(cop[2].x - cop[3].x);
}

