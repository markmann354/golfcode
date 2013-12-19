#include "posProject.h"


posProject::posProject(int maxRefWorldLines,
           int maxCameras,        // max number of images used to calibrate a camera position
           int maxObjects, //maxPoints+maxRefParallelLines+1,  // maxObjects = max patches we'll find, plus max parallel ref lines plus one for points in the fixed ref frame
           int lenX, int lenY, double aspect,  // image dims and full aspect ratio
           int pointsperline,
           int maxPositions,
           int maxRefWorldPoints,      // max points on a reference object: determined by the max 'fixed' world points we'll have
           int maxRefParallelLinesPlus,
           int fieldType,
           int bottom2top,
           double notionalPixHeight)
{
    myPoseEst = new bbcvp::PoseEstPanTiltHead(maxRefWorldLines,
                                       maxCameras,
                                       maxObjects,
                                       lenX, lenY, aspect,
                                       pointsperline,
                                       maxPositions,
                                       maxRefWorldPoints,      // max points on a reference object: determined by the max 'fixed' world points we'll have
                                       maxRefParallelLinesPlus);  // max number of reference objects (that can have lines on them) = 1 plus any lines with part-unknown coords

    myPoseEst->addNewImage(fieldType, bottom2top, notionalPixHeight);
    myPoseEst->clearLastImage();

}


posProject::posProject(bbcvp::PoseEstPanTiltHead *myPoseEst):myPoseEst(myPoseEst)
{}


int posProject::setPoseFromCam(string cameraFileName)
{
 /*****************Extracting Cam Params**********************************************************/

   bbcvp::Camera  *camp = new bbcvp::Camera();

   try {
      //printf("Reading in cam: %s\n", cameraFileName.c_str());
      vpcoreio::ReadCamera( *camp, cameraFileName.c_str());
      printf("Read");
   } catch (bbcvp::Error &e) {
      std::cerr << "Couldn't read camera : "<<e.GetMsg() <<"\n";
      return -1;
   }




   //qDebug("Got Cam File\n");

   double	pan = (*camp).GetPan();
   double	tilt	= (*camp).GetTilt();
   double	roll	= (*camp).GetRoll();
   double	f	= (*camp).FocalLength() ;
   double	sx 	= camp->PixelSizeX();
   double	sy 	= camp->PixelSizeY();

   cout << "pan "  << pan << endl;
   cout << "tilt "  << tilt << endl;
   cout << "roll "  << roll << endl;
   cout << "f "  << f << endl;
   cout << "sx"  << sx << endl;
   cout << "sy"  << sy << endl;
   //int width((*camp).Width()), height((*camp).Height());
   bbcvp::Vector3 C((*camp).C());
   double pos_x, pos_y, pos_z;
   pos_x = (*camp).C().GetX();
   pos_y = (*camp).C().GetY();
   pos_z = (*camp).C().GetZ();
   double radToDeg = 180.0 /  3.14159265;

   myPoseEst->setCamPos(pos_x, pos_y, pos_z);
   myPoseEst->setCamRotPTR((pan*radToDeg), (tilt*radToDeg), (roll*radToDeg));
   myPoseEst->setCamFocalLength(f);  // specify full height in field lines if interlaced, as PoseEStPanTiltHead uses ylen & YP for a field internally

   //This line is to set the world rotation. This information is NOT in the cam files so must be recorded and entered here.
   //Currently this can only be hardcoded in. To avoid this issue camera rotation should be allowed when tracking.
   //myPoseEst->setPitchRot(0, 0, 0.59);

   delete camp;
   return 1;
}




void posProject::setPlane(int intersectionXYZ, double intersectionPosition, int cam)
{
    this->intersectionXYZ = intersectionXYZ;
    this->intersectionPosition = intersectionPosition;
    this->cam = cam;

    cachedMat = false;
}


void posProject::lineOfSightWorld(double pixelPosX, double pixelPosY,double *wx, double *wy, double *wz)
{
  double x, y, z;
  double pos_x, pos_y, pos_z;
  double pos_x2, pos_y2, pos_z2;
  double x2, y2, z2;


  myPoseEst->getCamPos(&pos_x, &pos_y, &pos_z);
  myPoseEst->lineOfSight(pixelPosX, pixelPosY, &x, &y, &z);

  if (!cachedMat)
  {
     double xAngle = 0.0;
     double yAngle = 0.0;
     double zAngle = 0.0;

     /*myPoseEst->getPitchRot(&xAngle, &yAngle, &zAngle);


     xAngle = 0.0;
     yAngle = 0.0;
     zAngle = -0.010297443;*/


     double sw = sin(xAngle);
     double sq = sin(yAngle);
     double sk = sin(zAngle);
     double cw = cos(xAngle);
     double cq = cos(yAngle);
     double ck = cos(zAngle);

     double j0     = sq*cw;
     double j4     = sq*sw;
     rotMat[0]     = cq*ck;             // r11 (note (r11, r21, r31) is vector in dir of cam scanline
     rotMat[1]     = cq*sk;             // r21
     rotMat[2]     = -sq;               // r31
     rotMat[3]     = j4*ck - sk*cw;     // r12 (note (r12, r22, r32) is vector in dir of cam up
     rotMat[4]     = j4*sk + ck*cw;     // r22
     rotMat[5]     = cq*sw;             // r32
     rotMat[6]     = j0*ck + sk*sw;     // r13 (note (r13, r23, r33) is MINUS the view vector
     rotMat[7]     = j0*sk - ck*sw;     // r23
     rotMat[8]     = cq*cw;             // r33
     cachedMat = true;
}


  x2 = rotMat[0]*x + rotMat[3]*y + rotMat[6]*z;
  y2 = rotMat[1]*x + rotMat[4]*y + rotMat[7]*z;
  z2 = rotMat[2]*x + rotMat[5]*y + rotMat[8]*z;

  pos_x2 = rotMat[0]*pos_x + rotMat[3]*pos_y + rotMat[6]*pos_z;
  pos_y2 = rotMat[1]*pos_x + rotMat[4]*pos_y + rotMat[7]*pos_z;
  pos_z2 = rotMat[2]*pos_x + rotMat[5]*pos_y + rotMat[8]*pos_z;


  double timesBy = 1.0;

  if (intersectionXYZ == 0)
  {
     timesBy = fabs((pos_x2-intersectionPosition) / x2);
  }
  else if (intersectionXYZ == 1)
  {
     timesBy = fabs((pos_y2-intersectionPosition) / y2);
  }
  else if (intersectionXYZ == 2)
  {
     timesBy = fabs((pos_z2-intersectionPosition) / z2);
  }
  else
  {
          //error
  }


  *wx = pos_x2 + x2*timesBy;
  *wy = pos_y2 + y2*timesBy;
  *wz = pos_z2 + z2*timesBy;
}

void posProject::worldToPixel(double wx, double wy, double wz, double *pixelPosX, double *pixelPosY)
{
   //Perhaps come kind of caching could be used here to save on projecting?
   myPoseEst->projectIntoImageArbitraryPoint(wx, wy, wz, pixelPosX, pixelPosY, 1);
}

void posProject::getPose(bbcvp::PoseEstPanTiltHead *mainPoseEst)
{
    double pan, tilt, roll, fovy;
    double pos_x, pos_y, pos_z;

    myPoseEst->getCamPos(&pos_x, &pos_y, &pos_z);
    myPoseEst->getCamRotPTR(&pan, &tilt, &roll);
    myPoseEst->getCamFocalLength(&fovy);

    mainPoseEst->setCamPos(pos_x, pos_y, pos_z);
    mainPoseEst->setCamRotPTR((pan), (tilt), (roll));
    mainPoseEst->setCamFocalLength(fovy);
    //mainPoseEst->setPitchRot(0, 0, 0.59);
}
