
double topFlagRatio = (double)distanceFromFootOfFlag/(double)(heightOfCameraOnTower-heightOfFlag);
double bottomFlagRatio = (double)distanceFromFootOfFlag/(double)heightOfCameraOnTower;
double screenRatio = (double)((double)min_flag-(double)ball_pixel_y)/(double)((double)min_flag - (double)max_flag);
double thetaFlag = (atan(topFlagRatio) - atan(bottomFlagRatio));
double scaleFactor = (double)((double)min_flag-(double)max_flag)/((double)heightOfFlag*sin(3.142/2-atan(1/bottomFlagRatio)));
double thetaBall =  screenRatio*thetaFlag;
double thetaFromBallToTower = atan(1/bottomFlagRatio) - thetaBall;
double distanceFromTowerToBall = heightOfCameraOnTower/tan(thetaFromBallToTower);
double distanceToBallFromTowerTop = sqrt(distanceFromTowerToBall*distanceFromTowerToBall+heightOfCameraOnTower*heightOfCameraOnTower);
double pixelDistanceFromFlagX = ball_pixel_x - flag_line;
double distanceFromTowerTopToFlag = sqrt(distanceFromFootOfFlag*distanceFromFootOfFlag+heightOfCameraOnTower*heightOfCameraOnTower);
double depthScaleFactor = scaleFactor*distanceFromTowerToBall/distanceFromFootOfFlag;
double distanceRightOfHole = distanceToBallFromTowerTop*pixelDistanceFromFlagX/depthScaleFactor/distanceFromTowerTopToFlag;

double distanceShortOfHole = distanceFromTowerToBall-distanceFromFootOfFlag;
//  std::cout << "distanceShortOfHole: " << distanceShortOfHole << std::endl;

distanceToHole =  sqrt(distanceShortOfHole*distanceShortOfHole+distanceRightOfHole*distanceRightOfHole);
//  std::cout << "DISTANCE TO HOLE: " << distanceToHole << std::endl;

/*

std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
std::cout << "<AugmentedVideoPlayer>" << std::endl;
std::cout << "<Camera>viewCam<file>/usr/local/augmentedvideoplayer/serviceproducer/videos/football_someSmoothing.cdata</file>" << std::endl;
std::cout << "<offset>0</offset>" << std::endl;
std::cout << "</Camera>" << std::endl;
std::cout << "<Label3D>" << std::endl;
std::cout << "<objectName>object_0</objectName>" << std::endl;
std::cout << "<frameIn>-1</frameIn>" << std::endl;
std::cout << "<frameOut>102</frameOut>" << std::endl;
std::cout << "<label>Jimenez</label>" << std::endl;
std::cout << "<scaleX>1</scaleX>" << std::endl;
std::cout << "<scaleY>1</scaleY>" << std::endl;
std::cout << "<scaleZ>1</scaleZ>" << std::endl;
std::cout << "<posZ>-23.28</posZ>" << std::endl;
std::cout << "<posY>" << distanceShortOfHole << "</posY>" << std::endl;
std::cout << "<posX>" << distanceRightOfHole << "</posX>" << std::endl;
std::cout << "</Label3D>" << std::endl;
std::cout << "</AugmentedVideoPlayer> " << std::endl;
*/






ofstream myfile;
myfile.open ("golf_feed.xml");
myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
myfile << "<AugmentedVideoPlayer>" << std::endl;
myfile << "<Camera>viewCam<file>/usr/local/augmentedvideoplayer/serviceproducer/videos/football_someSmoothing.cdata</file>" << std::endl;
myfile << "<offset>0</offset>" << std::endl;
myfile << "</Camera>" << std::endl;

myfile << "<Label3D>" << std::endl;
myfile << "<objectName>object_" << i << "</objectName>" << std::endl;
myfile << "<frameIn>-1</frameIn>" << std::endl;
myfile << "<frameOut>102</frameOut>" << std::endl;
myfile << "<label>Jimenez</label>" << std::endl;
myfile << "<scaleX>1</scaleX>" << std::endl;
myfile << "<scaleY>1</scaleY>" << std::endl;
myfile << "<scaleZ>1</scaleZ>" << std::endl;
myfile << "<posZ>" << coordinatearray_z[ball_centre_x+screenWidth*ball_centre_y] << "</posZ>" << std::endl;
myfile << "<posY>" << coordinatearray_y[ball_centre_x+screenWidth*ball_centre_y] << "</posY>" << std::endl;
myfile << "<posX>" << coordinatearray_x[ball_centre_x+screenWidth*ball_centre_y] << "</posX>" << std::endl;
myfile << "</Label3D>" << std::endl;


myfile << "</AugmentedVideoPlayer> " << std::endl;
myfile.close();
