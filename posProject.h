#ifndef POSPROJECT_H
#define POSPROJECT_H


#include        <math.h>

//#include        "o_PicReadFileSeq.h"
#include        <vplibs/VPip/PatchCorrespondenceGen.h>
#include        <vplibs/VPcoreio/ReadImage.h>
#include        <vplibs/VPcoreio/ReadCamera.h>

using namespace std;


class posProject
{
private:
    bbcvp::PoseEstPanTiltHead *myPoseEst;

    int totalCached;
    int totalFailed;

    int cam;
    int intersectionXYZ;
    double intersectionPosition;
    double rotMat[9];
    bool cachedMat;


public:
    posProject(int maxRefWorldLines,
               int maxCameras,        // max number of images used to calibrate a camera position
               int maxObjects, //maxPoints+maxRefParallelLines+1,  // maxObjects = max patches we'll find, plus max parallel ref lines plus one for points in the fixed ref frame
               int lenX, int lenY, double aspect,  // image dims and full aspect ratio
               int pointsperline,
               int maxPositions,
               int maxRefWorldPoints,      // max points on a reference object: determined by the max 'fixed' world points we'll have
               int maxRefParallelLinesPlus,
               int fieldType,
               int bottom2top,
               double notionalPixHeight);

    posProject(bbcvp::PoseEstPanTiltHead *myPoseEst);

    posProject(){};

    //Read in from cameraFileName then set the PoseEstPanTiltHead with the suitable values
    int setPoseFromCam(string cameraFileName);

    //Set where points should project to in the real world. By default it is the floor
    //X = 0, Y = 1, Z = 2. intersectionPosition set the point on that axis.
    void setPlane(int intersectionXYZ = 1, double intersectionPosition = 0.0, int cam=-1);

    //Project from the pixel in the image to a point in the real world
    void lineOfSightWorld(double pixelPosX, double pixelPosY, double *wx, double *wy, double *wz);

    //Poject from the world to a pixel in the image
    void worldToPixel(double wx, double wy, double wz, double *pixelPosX, double *pixelPosY);


    void getPose(bbcvp::PoseEstPanTiltHead *mainPoseEst);
};

#endif // POSPROJECT_H
