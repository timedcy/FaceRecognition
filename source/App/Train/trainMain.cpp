/**
* @mainpage LWIFace
*   <b>Codes to detect faces</b>
*
*   LWIFace reads the model from Cov_eye and Cov_face. Then it uses the
*   model to detect faces.
*
*   inputfile name: testinput.txt
*   input image folder: input/
*   output image folder: output/
*
*   @author unspecified
*/

/** \file FERAmain.cpp
    \brief Defines the entry point for the console application.

    Details.
*/

#ifdef _WIN32
#include <io.h>
#endif

#include <stdio.h>
#include <highgui.h>
#include <math.h>
#include <cv.h>
#include <time.h>

#include "TLibCommon/EyesDetector.h"
#include "TLibCommon/Detector.h"
#include "TLibCommon/global.h"
#include <fstream>
#include "TLibCommon/ConvNN.h"
//#include "TLibCommon/CvGaborFace.h"

#include "TLibCommon/faceFeature.h"
#include "TLibCommon/affineWarping.h"
#include <cxcore.h>
#include "Global.h"
#include "svmImage.h"

#define WRITE_FEATURE_DATUM_2_FILE
#define TRAIN_IMAGE_DIR			"../../image/train/"
#define	STR_INPUT_IMAGE_DIR		"../../image/train/input_align_2/"
#define TRAIN_INPUT_TXT_FILE    "../../image/trainIn.txt"
#define TRAIN_OUTPUT_TXT_FILE	"../../image/trainOut.txt"
#define BIN_FILE				"../../image/faces.bin"
#define LGT_BIN_FILE			"../../image/LGT.bin"
#define IMAGE_TAG_DIR			"../../image/ImgTag/"
#define WEIGHTS_BIN				"../../image/weight.bin"
#define SVM_TRAIN_BIN_FILE		"../../image/svmTrain.bin"
#define SVM_LABEL_BIN_FILE		"../../image/svmLabel.bin"
#define SVM_LIST_DIR			"../../image/svm/"
#define	LBP_FEATURE_DIR			"../../image/LBP/"

//#define DO_MATCH
//#define	STR_INPUT_IMAGE_DIR		"Query/"
//#define	STR_INPUT_IMAGE_DIR		"input_align/"

// demo only
#define MAX_NUM_FACE_ID_TAG			MAX_FACE_ID

using namespace std;

int    NSAMPLES = 1;
int    MAX_ITER = 1;
int    NTESTSAMPLES = 1;

#define BUFSIZE 20




#if 0
#pragma  comment(lib, "opencv_calib3d242d.lib")
#pragma  comment(lib, "opencv_contrib242d.lib")
#pragma  comment(lib, "opencv_core242d.lib")

#pragma  comment(lib, "opencv_features2d242d.lib")
#pragma  comment(lib, "opencv_flann242d.lib")

#pragma  comment(lib, "opencv_gpu242d.lib")
#pragma  comment(lib, "opencv_highgui242d.lib")

#pragma  comment(lib, "opencv_legacy242d.lib")
#pragma  comment(lib, "opencv_ml242d.lib")

#pragma  comment(lib, "opencv_objdetect242d.lib")

#pragma  comment(lib, "opencv_ts242d.lib")
#pragma  comment(lib, "opencv_video242d.lib")
#pragma  comment(lib, "opencv_imgproc242d.lib")

#endif


FACE3D_Type			gf;
SVM_GST				gst;

//init svm classifier
svm_classifer_clean<int,double> svm;


unitFaceFeatClass	*bufferSingleFeatureID;

void showResults(IplImage * frame, FACE3D_Type * gf);
void processFileList();
void trainLGT(FACE3D_Type *gf);
void trainWeight(FACE3D_Type *gf);
void trainWeightForLBP(FACE3D_Type *gf);
void veriTrain( FACE3D_Type* gf);
void generateSVMList(FACE3D_Type* gf);
void svmTrainFromList(int start, int end);
void extractFeatureFromImage(FACE3D_Type* gf, float* feature, IplImage* pFrame, eyesDetector * detectEye, faceDetector * faceDet);
void prepareLBPFeaturesToFile( FACE3D_Type* gf);
void boostSVMTrain();

// shuffle array to randomly select group members
void shuffle(int *list, int n) 
{    
    srand((unsigned) time(NULL)); 
	int t, j;


    if (n > 1) 
	{
        int i;
        for (i = n - 1; i > 0; i--) 
		{
            j = i + rand() / (RAND_MAX / (n - i) + 1);
            t = list[j];
            list[j] = list[i];
            list[i] = t;
        }
    }
}

int testLiveFace()
{

	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int    lineWidth=2;
	IplImage* pFrame = NULL; 
	IplImage* orgFace = NULL;

	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, hScale,vScale,0,lineWidth);
	eyesDetector * detectEye = new eyesDetector;

	faceDetector * faceDet =  new faceDetector();
	IplImage*  gray_face_CNN = cvCreateImage(cvSize(CNNFACECLIPHEIGHT,CNNFACECLIPWIDTH), 8, 1);

	// Feature points array
	CvPoint pointPos[6];
	CvPoint *leftEye    = &pointPos[0];
	CvPoint *rightEye   = &pointPos[2];
	CvPoint *leftMouth  = &pointPos[4];
	CvPoint *rightMouth = &pointPos[5];

	IplImage *frame = 0;
	CvCapture *capture  = 0;
	capture = cvCaptureFromCAM(-1);

	frame = cvQueryFrame( capture );

	cvNamedWindow("show");

	//Count image numbers
	static int frameNum = 0;

	while(1)
	{

		frame = cvQueryFrame( capture );
		if ( !frame )	break;


		pFrame = frame;

		if( faceDet->runFaceDetector(pFrame))
		{

			IplImage * clonedImg = cvCloneImage(pFrame);

			detectEye->runEyeDetector(clonedImg, gray_face_CNN, faceDet, pointPos);

			cvReleaseImage(&clonedImg);

			int UL_x = faceDet->faceInformation.LT.x;
			int UL_y = faceDet->faceInformation.LT.y;

			// face width and height
			CvPoint pt1, pt2;
			pt1.x =  faceDet->faceInformation.LT.x;
			pt1.y = faceDet->faceInformation.LT.y;
			pt2.x = pt1.x + faceDet->faceInformation.Width;
			pt2.y = pt1.y + faceDet->faceInformation.Height;

			cvRectangle(pFrame, pt1, pt2, cvScalar(0,0,255),2, 8, 0);

			cvCircle(pFrame, leftEye[0],  2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, leftEye[1],  2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, rightEye[0], 2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, rightEye[1], 2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, *leftMouth,  2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, *rightMouth,  2, cvScalar(255,0,0), -1);

			printf("(%d,%d) (%d,%d) (%d,%d) (%d,%d) (%d,%d)"
				" (%d,%d) (%d,%d) (%d,%d)\n",\
				pt1.x, pt1.y, pt2.x, pt2.y,
				pointPos[0].x,pointPos[0].y,
				pointPos[1].x,pointPos[1].y,
				pointPos[2].x,pointPos[2].y,
				pointPos[3].x,pointPos[3].y,
				pointPos[4].x,pointPos[4].y,
				pointPos[5].x,pointPos[5].y);

		}
		else
		{
			cvPutText(pFrame, "NO Face Found", cvPoint(500,30), &font, cvScalar(255,255,255));
		}

		cvShowImage("show", pFrame);

		frameNum++;

		if (cvWaitKey(1) == 'q') exit;
	}

	return NULL;
}


int testVideoData2()
{
	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int    lineWidth=2;
	int    validFaces = 0;
	

	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, hScale,vScale,0,lineWidth);

//	string faceData = "yalebfaceFileList.txt";//"lwifaceFileList.txt";
//	CvGaborFace * gFace = new CvGaborFace;
//	gFace->loadTrnList(faceData);


	eyesDetector * detectEye = new eyesDetector;
#if 0
	CvCapture* capture = cvCaptureFromCAM( 0 );
#elif 0
	CvCapture* capture = cvCaptureFromAVI("D:\\out_york1.avi");
#endif
	//cvNamedWindow("show");

	printf("Write out face images with feature points marked\n=============\n");

	faceDetector * faceDet =  new faceDetector();
//	CvMat*  face_data = cvCreateMat( 1, CNNFACECLIPHEIGHT*CNNFACECLIPWIDTH, CV_32FC1 );
//	CvMat*  face_data_int = cvCreateMat( 1, CNNFACECLIPHEIGHT*CNNFACECLIPWIDTH, CV_8UC1 );
	IplImage*  gray_face_CNN = cvCreateImage(cvSize(CNNFACECLIPHEIGHT,CNNFACECLIPWIDTH), 8, 1);

	// Feature points array
	CvPoint pointPos[6];
	CvPoint *leftEye    = &pointPos[0];
	CvPoint *rightEye   = &pointPos[2];
	CvPoint *leftMouth  = &pointPos[4];
	CvPoint *rightMouth = &pointPos[5];

#if 0
	// Image list.
	FILE *fpinput = fopen(TRAIN_INPUT_TXT_FILE,"r");
	if(fpinput==NULL){
		printf("open file testinput.txt failed!\n");fflush(stdout);
		exit(-1);
	}
	FILE *fpoutput = fopen(TRAIN_OUTPUT_TXT_FILE,"w");
	if(fpoutput==NULL){
		printf("open file testinput.txt failed!\n");fflush(stdout);
		exit(-1);
	}
#endif

	//char inputdir[100]="input_align_2/";
	char inputdir[100]= STR_INPUT_IMAGE_DIR;
	char tmpname[500];

	char tagImgdir[100]= IMAGE_TAG_DIR;
	char tagImgName[500];
	int tmpFaceID;
	int matchedFaceID;
	int	i;

	//Count image numbers
	static int frameNum = 0;

	// demo display.

	IplImage *	imgFaceIDTag[MAX_NUM_FACE_ID_TAG];
	IplImage *	inputQueryImgResized;
	int			numTaggedFaces = 0;

	for (i=0; i<MAX_NUM_FACE_ID_TAG; i++)
	{
		sprintf( tagImgName, "%s%d.JPG",tagImgdir, i+1);
		imgFaceIDTag[i]		= cvLoadImage(tagImgName,3);

		if (imgFaceIDTag[i] == NULL)
		{
			printf("\nNum of tagged faces: %d\n", i);fflush(stdout);
			//exit(-1);
			numTaggedFaces = i;
			break;
		}
	
	}

	gf.numIDtag = numTaggedFaces;

	inputQueryImgResized	= cvCreateImage(cvSize(960, 720), IPL_DEPTH_8U, 3);;

	// Face feature archive.
	bufferSingleFeatureID	= (unitFaceFeatClass *)malloc( sizeof(unitFaceFeatClass) );

#ifdef WRITE_FEATURE_DATUM_2_FILE
	FILE *fpOutBinaryFile = fopen( BIN_FILE, "w+b");  // 2013.2.11 a+b to w+b
#endif

	/************************************************************************/
	/* look over face image samples.                                        */
	/************************************************************************/
	char to_search[500] = "./";
	// for each face tag
	for ( int ii = 0; ii < numTaggedFaces; ii++)
	{
		sprintf(to_search, "%s%d/*.jpg",TRAIN_IMAGE_DIR, ii+1);
		long handle;                                                //search handle
		struct _finddata_t fileinfo;                          // file info struct
		handle=_findfirst(to_search,&fileinfo);         
		if(handle != -1) 
		{
			do
			{
		//printf("%s\n",fileinfo.name);                        
		
		//printf("%s\n",fileinfo.name);
		        
	//while(!feof(fpinput)){

#if 0
		strcpy(tmpname,"");
		fgets(tmpname,500,fpinput);
		//printf(" %d ", strlen(tmpname)); 
		if(strlen(tmpname)==0)
			break;
		tmpname[strlen(tmpname)-1]='\0';
		puts(tmpname);
#endif
		//fscanf(fpinput, "%s	%d", tmpname, &tmpFaceID);

		// Store captured frame
		IplImage* pFrame = NULL; 
		IplImage* orgFace = NULL;

		

		//Image ID
		//fscanf(fpinput, "%d", &tmpFaceID);

#if 0
		pFrame = cvQueryFrame( capture );
#else
		char tmppath[500];
		//sprintf(tmppath,"%s%s",inputdir,tmpname);
		sprintf(tmppath,"%s",fileinfo.name);
		puts(tmppath);
		sprintf(tmppath,"%s%d/%s",TRAIN_IMAGE_DIR, ii+1, fileinfo.name);
		pFrame = cvLoadImage(tmppath,3);
		if(pFrame == NULL)
		{
			printf("read image file error!\n");fflush(stdout);
			exit(-1);
		}
#endif

		if( faceDet->runFaceDetector(pFrame))
		{	
			/* there was a face detected by OpenCV. */
			
			IplImage * clonedImg = cvCloneImage(pFrame);

			detectEye->runEyeDetector(clonedImg, gray_face_CNN, faceDet, pointPos);

			cvReleaseImage(&clonedImg);

			int UL_x = faceDet->faceInformation.LT.x;
			int UL_y = faceDet->faceInformation.LT.y;

			// face width and height
			CvPoint pt1, pt2;
			pt1.x =  faceDet->faceInformation.LT.x;
			pt1.y = faceDet->faceInformation.LT.y;
			pt2.x = pt1.x + faceDet->faceInformation.Width;
			pt2.y = pt1.y + faceDet->faceInformation.Height;

			// face warping.

			IplImage * tarImg = cvCreateImage( cvSize(warpedImgW, warpedImgH), IPL_DEPTH_8U, warpedImgChNum );
			
#if 0 // disabled 2013.2.8
			faceWarping(	pFrame,  tarImg,
							( leftEye[0].y + leftEye[1].y )/2, ( leftEye[0].x + leftEye[1].x )/2,
							( rightEye[0].y + rightEye[1].y )/2, ( rightEye[0].x + rightEye[1].x )/2,
							( leftMouth[0].y), ( leftMouth[0].x),
							( rightMouth[0].y), ( rightMouth[0].x),
							FIXED_LEFT_EYE_Y, FIXED_LEFT_EYE_X,
							FIXED_RIGHT_EYE_Y, FIXED_RIGHT_EYE_X,
							FIXED_LEFT_MOUTH_Y, FIXED_LEFT_MOUTH_X,
							FIXED_RIGHT_MOUTH_Y, FIXED_RIGHT_MOUTH_X);
#endif
#if 0 // disabled 2013.2.11
			//2013.2.8 simply apply rotation 
			cv::Point2f src_center(faceDet->faceInformation.Width/2.0F, faceDet->faceInformation.Height/2.0F);
			int lEyeCenterY = ( leftEye[0].y + leftEye[1].y )/2, lEyeCenterX = ( leftEye[0].x + leftEye[1].x )/2;
			int rEyeCenterY = ( rightEye[0].y + rightEye[1].y )/2, rEyeCenterX = ( rightEye[0].x + rightEye[1].x )/2;

			//
			double tanAngle = 1.0 * (rEyeCenterY - lEyeCenterY)/(rEyeCenterX - lEyeCenterX);  
			double angle = 90 * atan( tanAngle ); //small adjustment: only take half adjust degree 180--��90


			IplImage * clonedImg2 = cvCloneImage(pFrame);
			cvSetImageROI( clonedImg2, cvRect(pt1.x, pt1.y, faceDet->faceInformation.Width, faceDet->faceInformation.Height));
			IplImage* faceFrame = cvCreateImage(cvSize(faceDet->faceInformation.Width, faceDet->faceInformation.Height), IPL_DEPTH_8U, warpedImgChNum );
			cvCopy( clonedImg2, faceFrame);
			cvReleaseImage(&clonedImg2);

			//sprintf(tmppath,"output_align/facecrop_%s",tmpname);
			//cvSaveImage(tmppath,faceFrame);

			tarImg = faceRotate( angle, src_center.x, src_center.y, faceFrame, false, warpedImgW, warpedImgH);

			cvReleaseImage(&faceFrame);
#endif
			//2013.2.11 face rotation - Zhi
			faceRotate(leftEye, rightEye, pFrame, tarImg, faceDet->faceInformation.Width, faceDet->faceInformation.Height);

			//convert to gray and downsampling
			
			grayDownsample(tarImg, &gf, frameNum, FALSE);

			// feature extraction.
			gf.featureLength = 0;

#if USE_CA
			extractCAFeature(&gf);
#endif

#if USE_GBP
			extractGBPFaceFeatures( (unsigned char*)(tarImg->imageData), (tarImg->widthStep), &gf);
#endif
#if USE_LBP
			extractLBPFaceFeatures( (unsigned char*)(tarImg->imageData), (tarImg->widthStep), &gf, FALSE);
#endif
#if USE_GABOR
			extractGaborFeatures( &gf );
#endif


#if 0
			//Debug only 
			int WW = gf.tWidth/2 ;
			int HH = gf.tHeight/2;
			
			FILE* f1 = fopen("../../image/debug/f2.bin","wb");
			fwrite(gf.fImage1, sizeof(int), WW*HH, f1);
			fclose(f1);
#endif

#ifdef WRITE_FEATURE_DATUM_2_FILE

			// write feature to a binary file.
			bufferSingleFeatureID->id	= ii+1;
			memcpy( bufferSingleFeatureID->feature, gf.faceFeatures, sizeof(float)*TOTAL_FEATURE_LEN );
#if DEBUG_MODE
			sprintf(bufferSingleFeatureID->imagename, "%s%d/%s",TRAIN_IMAGE_DIR, ii+1, fileinfo.name);
#endif

#if 0
			//debug only - output histogram
			char tmpHistPath[500];
			sprintf(tmpHistPath, "../../image/Debug/%s.txt", fileinfo.name);
			FILE* debugFile = fopen( tmpHistPath, "w+");
			int tmpPtr = 0;
			while (tmpPtr < FACE_FEATURE_LEN )
			{
				for (int ii = 0; ii < 256; ii++)
				{
					fprintf(debugFile, "%d	", (int) gf.faceFeatures[tmpPtr + ii]);
				}
				fprintf(debugFile, "\n");
				tmpPtr += 256;
			}
			fclose(debugFile);
#endif

			fwrite( bufferSingleFeatureID, 1, sizeof(unitFaceFeatClass), fpOutBinaryFile );
#endif

#if USE_WEIGHT
			gf.bufferFaceFeatures[validFaces] = *bufferSingleFeatureID;
#endif

#ifdef DO_MATCH

			matchedFaceID	= matchFace( gf.faceFeatures, &gf );
#endif

			//system("pause");
			//sprintf(tmppath,"output_align/warped_%s",tmpname);

			//cvSaveImage(tmppath,tarImg);
			//

			cvReleaseImage(&tarImg);


			// plot graphic results.
			cvRectangle(pFrame, pt1, pt2, cvScalar(0,0,255),2, 8, 0);

			int lEyeCenterY = ( leftEye[0].y + leftEye[1].y )/2, lEyeCenterX = ( leftEye[0].x + leftEye[1].x )/2;
			int rEyeCenterY = ( rightEye[0].y + rightEye[1].y )/2, rEyeCenterX = ( rightEye[0].x + rightEye[1].x )/2;
			CvPoint lEyeball = cvPoint(lEyeCenterX, lEyeCenterY);
			CvPoint rEyeball = cvPoint(rEyeCenterX, rEyeCenterY);

			//modified: only show eyeballs position
			cvCircle(pFrame, lEyeball,  10, cvScalar(255,0,0), -1);
			cvCircle(pFrame, rEyeball,  10, cvScalar(255,0,0), -1);

			//cvCircle(pFrame, leftEye[0],  10, cvScalar(255,0,0), -1);
			//cvCircle(pFrame, leftEye[1],  10, cvScalar(255,0,0), -1);
			//cvCircle(pFrame, rightEye[0], 10, cvScalar(255,0,0), -1);
			//cvCircle(pFrame, rightEye[1], 10, cvScalar(255,0,0), -1);
			//cvCircle(pFrame, *leftMouth,  10, cvScalar(255,0,0), -1);
			//cvCircle(pFrame, *rightMouth, 10, cvScalar(255,0,0), -1);

			//save labeled pFrame
			//sprintf(tmppath,"output_align/labeled_%s",tmpname);
			//cvSaveImage(tmppath,pFrame);

			//fprintf(fpoutput,"%s: (%d,%d) (%d,%d) (%d,%d) (%d,%d) (%d,%d)"
			//	" (%d,%d) (%d,%d) (%d,%d)\n",tmpname,\
			//	pt1.x, pt1.y, pt2.x, pt2.y,
			//	pointPos[0].x,pointPos[0].y,
			//	pointPos[1].x,pointPos[1].y,
			//	pointPos[2].x,pointPos[2].y,
			//	pointPos[3].x,pointPos[3].y,
			//	pointPos[4].x,pointPos[4].y,
			//	pointPos[5].x,pointPos[5].y);

//			cvReleaseImage(&normalizedface);
//			cvReleaseImage(&grayFace);

#ifdef DO_MATCH
			// debug:
			printf("\nMatched ID: %d \n------------------\n", matchedFaceID);
			//printf("\nAdjusted angle: %.4f \n-------------------\n", angle);
			fprintf(fpoutput, "\n\n%s:	:		%d	%d\n\n", tmpname, tmpFaceID, matchedFaceID);

			cvResize(pFrame, inputQueryImgResized, 1);
			cvNamedWindow("Input Image");
			cvShowImage("Input Image", inputQueryImgResized);

			cvNamedWindow("Matched Face");
			cvShowImage("Matched Face", imgFaceIDTag[matchedFaceID-1]);
			cvWaitKey(100);
#endif
		validFaces++;
		}
		else
		{
			cvPutText(pFrame, "NO Face Found", cvPoint(500,30), &font, cvScalar(255,255,255));
		}

		/************************************************************************/
		/* Display                                                              */
		/************************************************************************/
#if 0
		cvResize(pFrame, inputQueryImgResized, 1);
		cvNamedWindow("Input Image");
		cvShowImage("Input Image", inputQueryImgResized);

		cvNamedWindow("Matched Face");
		cvShowImage("Matched Face", imgFaceIDTag[matchedFaceID-1]);
		cvWaitKey(100);

#endif
		// save original image.
		//sprintf(tmppath,"output_align/%s",tmpname);
		//cvSaveImage(tmppath,pFrame);
		//system("pause");
	
		cvReleaseImage(&pFrame);


		frameNum++;

		if (cvWaitKey(1) == 'q')
			exit;
	}while(_findnext(handle,&fileinfo) == 0);               
	
	_findclose(handle); 
	//fclose(fpinput);
	//fclose(fpoutput);
	}
	}
	
#ifdef WRITE_FEATURE_DATUM_2_FILE
	fclose(fpOutBinaryFile);
#endif

	//record valid faces
	gf.validFaces = validFaces;
	return NULL;
	
}



int getSampleFaceFeatures()
{

	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int    lineWidth=2;

	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, hScale,vScale,0,lineWidth);

	eyesDetector * detectEye = new eyesDetector;

	cvNamedWindow("show");

	faceDetector * faceDet =  new faceDetector();
	IplImage*  gray_face_CNN = cvCreateImage(cvSize(CNNFACECLIPHEIGHT,CNNFACECLIPWIDTH), 8, 1);


	// Feature points array
	CvPoint pointPos[6];
	CvPoint *leftEye    = &pointPos[0];
	CvPoint *rightEye   = &pointPos[2];
	CvPoint *leftMouth  = &pointPos[4];
	CvPoint *rightMouth = &pointPos[5];


	FILE *fpinput = fopen("testinput_align.txt","r");
	if(fpinput==NULL){
		printf("open file testinput.txt failed!\n");fflush(stdout);
		exit(-1);
	}
	FILE *fpoutput = fopen("testoutput_align.txt","w");
	if(fpoutput==NULL){
		printf("open file testinput.txt failed!\n");fflush(stdout);
		exit(-1);
	}


	char inputdir[100]="input_align/";
	char tmpname[500];
	while(!feof(fpinput)){
		strcpy(tmpname,"");
		fgets(tmpname,500,fpinput);
		//printf(" %d ", strlen(tmpname)); 
		if(strlen(tmpname)==0)
			break;
		tmpname[strlen(tmpname)-1]='\0';
		puts(tmpname);

		IplImage* pFrame = NULL; // Store captured frame
		IplImage* orgFace = NULL;

		//Count image numbers
		static int frameNum = 0;

#if 0
		pFrame = cvQueryFrame( capture );
#else
		char tmppath[500];
		sprintf(tmppath,"%s%s",inputdir,tmpname);
		puts(tmppath);
		pFrame = cvLoadImage(tmppath,3);
		if(pFrame == NULL)
		{
			printf("read image file error!\n");fflush(stdout);
			exit(-1);
		}
#endif

		if( faceDet->runFaceDetector(pFrame))
		{

			IplImage * clonedImg = cvCloneImage(pFrame);

			detectEye->runEyeDetector(clonedImg, gray_face_CNN, faceDet, pointPos);

			cvReleaseImage(&clonedImg);

			int UL_x = faceDet->faceInformation.LT.x;
			int UL_y = faceDet->faceInformation.LT.y;

			// face width and height
			CvPoint pt1, pt2;
			pt1.x =  faceDet->faceInformation.LT.x;
			pt1.y = faceDet->faceInformation.LT.y;
			pt2.x = pt1.x + faceDet->faceInformation.Width;
			pt2.y = pt1.y + faceDet->faceInformation.Height;

			cvRectangle(pFrame, pt1, pt2, cvScalar(0,0,255),2, 8, 0);

			cvCircle(pFrame, leftEye[0],  2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, leftEye[1],  2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, rightEye[0], 2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, rightEye[1], 2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, *leftMouth,  2, cvScalar(255,0,0), -1);
			cvCircle(pFrame, *rightMouth,  2, cvScalar(255,0,0), -1);
			fprintf(fpoutput,"%s: (%d,%d) (%d,%d) (%d,%d) (%d,%d) (%d,%d)"
				" (%d,%d) (%d,%d) (%d,%d)\n",tmpname,\
				pt1.x, pt1.y, pt2.x, pt2.y,
				pointPos[0].x,pointPos[0].y,
				pointPos[1].x,pointPos[1].y,
				pointPos[2].x,pointPos[2].y,
				pointPos[3].x,pointPos[3].y,
				pointPos[4].x,pointPos[4].y,
				pointPos[5].x,pointPos[5].y);

//			cvReleaseImage(&normalizedface);
//			cvReleaseImage(&grayFace);
		}
		else
		{
			cvPutText(pFrame, "NO Face Found", cvPoint(500,30), &font, cvScalar(255,255,255));
		}

		cvShowImage("show", pFrame);

		sprintf(tmppath,"output_align/%s",tmpname);
		cvSaveImage(tmppath,pFrame);
	

		frameNum++;

		if (cvWaitKey(1) == 'q')
			exit(0);
	}
	fclose(fpinput);
	fclose(fpoutput);
	return NULL;
}




int main(int argc, char** argv)
{
	//-------------------
	// initializations.
	//-------------------
	//boostSVMTrain();
	//svmTrainFromList(0, 8);

	initFaceWarping();	
	initFaceFeature( &gf, 80, 80);
#ifdef DO_MATCH
	loadFaceData( &gf );
#endif

	//-------------------
	// data access.
	processFileList();
	prepareLBPFeaturesToFile(&gf);
	
	//generateSVMList(&gf);
	//-------------------

	//trainLGT(&gf);
	//trainWeight(&gf);	// train weight for histogram
	testVideoData2();	// find the face coordinates and eye, mouse position

#if USE_WEIGHT
	//veriTrain( &gf );
	trainWeightForLBP(&gf);
#endif
	
	//videoAnalysis();	// extract feature given the face coordinates

	//-------------------
	// closing.
	//-------------------
	//closeFaceWarping();
	freeFaceFeature(&gf);
	system("pause");
	
}

void showResults(IplImage * frame, FACE3D_Type * gf)
{
	unsigned char * ptr, *mask;
	int r, c, widthStep;
	int FRAME_WIDTH, FRAME_HEIGHT;

	FRAME_HEIGHT = gf->FRAME_HEIGHT;
	FRAME_WIDTH = gf->FRAME_WIDTH;

	widthStep = frame->widthStep;
	mask = gf->mask;

	for(r=0; r<FRAME_HEIGHT; r++)
	for(c=0; c<FRAME_WIDTH; c++)
	{
		ptr = (unsigned char *)(frame->imageData + r * widthStep + c * 3);

		if(*(mask + r * FRAME_WIDTH + c) == 1)
		{
			ptr[0] = 255; ptr[1] = 255;
		}
	
	}


}


//-------------------------------------------------------------------
void processFileList()
{
	char inputdir[100]= STR_INPUT_IMAGE_DIR;

	char tagImgdir[100]= IMAGE_TAG_DIR;
	char tagImgName[500];
	int	i;
	

	// count how many face IDs in training list

	IplImage *	imgFaceIDTag[MAX_NUM_FACE_ID_TAG];
	int			numTaggedFaces = 0;

	for (i=0; i<MAX_NUM_FACE_ID_TAG; i++)
	{
		sprintf( tagImgName, "%s%d.JPG",tagImgdir, i+1);
		imgFaceIDTag[i]		= cvLoadImage(tagImgName,3);

		if (imgFaceIDTag[i] == NULL)
		{
			printf("\nNum of tagged faces: %d\n", i);fflush(stdout);
			numTaggedFaces = i;
			break;
		}
	
	}
	gf.numIDtag = numTaggedFaces;

	//process image list
	char to_search[260];
	int  numImgs = 0;
	// for each face tag
	for ( int ii = 0; ii < numTaggedFaces; ii++)
	{
		sprintf(to_search, "%s%d/*.jpg",TRAIN_IMAGE_DIR, ii+1);
		long handle;                                                //search handle
		struct _finddata_t fileinfo;                          // file info struct
		handle=_findfirst(to_search,&fileinfo);         
		if(handle != -1) 
		{
			do
			{
				char *tmpPath = &gf.fileList.fileName[numImgs][0];
				sprintf(tmpPath,"%s%d/%s",TRAIN_IMAGE_DIR, ii+1, fileinfo.name);
				gf.fileList.fileID[numImgs] = ii+1;
				numImgs ++;
			}while(_findnext(handle,&fileinfo) == 0);               
	
			_findclose(handle);
		}
	}
	assert(numImgs < MAX_INPUT_IMAGES);
	gf.fileList.listLength = numImgs; // store list length
				
}




/* train LGT features */
void trainLGT(FACE3D_Type *gf)
{
	int		numImagesInList = gf->fileList.listLength;
	int		regionH = gf->tHeight / gf->LGTCenters.numRegionH;
	int		regionW = gf->tWidth / gf->LGTCenters.numRegionW;
	int		i,j,m,n,k,l;
	char	*tmpPath;
	int		gaborWSize = gf->gaborWSize;
	float	*gaborResponse = gf->gaborResponse;
	//float	*tmpGaborResponse;
	unsigned char *tmpImageData;
	int		ptr,centersPtr,idx;
	int		stepPixel = gf->gaborStepPixel; 
	int		stepWidth = gf->gaborStepWidth;
	int		fl[warpedImgH][warpedImgW];	
	float	dist,tmpDist,minDist;
	int		rRegion, cRegion;
	unitFaceFeatClass	*bufferSingleFeatureID;
	int		curFrame = 0;

	//init
	IplImage *pFrame;
	bufferSingleFeatureID	= (unitFaceFeatClass *)malloc( sizeof(unitFaceFeatClass) );
	IplImage *tarFrame = cvCreateImage( cvSize(warpedImgW, warpedImgH), IPL_DEPTH_8U, warpedImgChNum );
	IplImage *grayFrame = cvCreateImage(cvSize(warpedImgW, warpedImgH), IPL_DEPTH_8U, 1 );
	//tmpGaborResponse = gf->tmpGaborResponse;
	tmpImageData = gf->tmpImageData;
	eyesDetector * detectEye = new eyesDetector;
	faceDetector * faceDet =  new faceDetector();
	IplImage*  gray_face_CNN = cvCreateImage(cvSize(CNNFACECLIPHEIGHT,CNNFACECLIPWIDTH), 8, 1);
	// Feature points array for eyes detection
	CvPoint pointPos[6];
	CvPoint *leftEye    = &pointPos[0];
	CvPoint *rightEye   = &pointPos[2];
	CvPoint *leftMouth  = &pointPos[4];
	CvPoint *rightMouth = &pointPos[5];

	// open binary file to write
	FILE *fb = fopen( BIN_FILE, "w+b");
	if(fb==NULL){
		printf("open file faces.bin failed!\n");fflush(stdout);
		exit(-1);
	}

	for ( i = 0; i < numImagesInList; i++)
	{
		tmpPath = &gf->fileList.fileName[i][0];
		puts(tmpPath);
		pFrame = cvLoadImage(tmpPath, 3);
		if(pFrame == NULL)
		{
			printf("read image file error!\n");fflush(stdout);
			exit(-1);
		}
		if( faceDet->runFaceDetector(pFrame))
		{	
			/* there was a face detected by OpenCV. */
			
			IplImage * clonedImg = cvCloneImage(pFrame);

			detectEye->runEyeDetector(clonedImg, gray_face_CNN, faceDet, pointPos);

			cvReleaseImage(&clonedImg);

			int UL_x = faceDet->faceInformation.LT.x;
			int UL_y = faceDet->faceInformation.LT.y;

			
			//align face
			faceRotate(leftEye, rightEye, pFrame, tarFrame, faceDet->faceInformation.Width, faceDet->faceInformation.Height);
			cvCvtColor(tarFrame, grayFrame, CV_RGB2GRAY);
			//get unsigned char image data from IplImage
			for ( m = 0; m < gf->tHeight; m++)
			{
				for ( n =0; n < gf->tWidth; n++)
				{
					tmpImageData[ m * gf->tWidth + n] = CV_IMAGE_ELEM( grayFrame, unsigned char, m, n );
				}
			}
			
			extractLGTFeatures(gf);
			
			bufferSingleFeatureID->id	= gf->fileList.fileID[i];
			memcpy( bufferSingleFeatureID->feature, gf->faceFeatures, sizeof(float)*TOTAL_FEATURE_LEN );
#if USE_WEIGHT
			gf->bufferFaceFeatures[curFrame] = *bufferSingleFeatureID;
#endif
			fwrite( bufferSingleFeatureID, 1, sizeof(unitFaceFeatClass), fb );
			curFrame++;
		}
	}
	gf->validFaces = curFrame;
	fclose(fb);

					

}


void trainWeight(FACE3D_Type *gf)
{
	int numIntra, numExtra;
	float *weight = gf->histWeight;
	float tmpWeight;
	float sumIntra, sumExtra;
	float sumIntraVar, sumExtraVar;
	float meanIntra, meanExtra;
	float varIntra, varExtra;
	int i, j, m, n;
	int numRegionH = gf->LGTCenters.numRegionH;
	int numRegionW = gf->LGTCenters.numRegionW;
	int numRegions = numRegionH * numRegionW;	//num of regions
	int histLength = gf->LGTCenters.numCenters;	//histogram length in each region
	int numFaces = gf->validFaces;
	float	dist,tmpDist, h1, h2;

	//float *tmpWeight = (float *)malloc(sizeof(float) * numRegions);

	for ( i = 0; i < numRegions; i++)
	{
		//for each region
		//Calculate means
		numIntra = 0;
		numExtra = 0;
		sumIntra = 0;
		sumExtra = 0;
		sumIntraVar = 0;
		sumExtraVar = 0;
		for ( m = 0; m < numFaces; m++)
		{
			for ( n = m + 1; n < numFaces; n++)
			{
				dist = 0;
				for ( j = 0; j < histLength; j++)
				{
					//compute chi-square distance
					h1 = gf->bufferFaceFeatures[m].feature[ histLength * i + j];
					h2 = gf->bufferFaceFeatures[n].feature[ histLength * i + j];
					if ( h1 != 0 || h2 !=0)
					{
						tmpDist = h1 - h2;
						dist += tmpDist * tmpDist / ( h1 + h2);
					}
				}
				if (gf->bufferFaceFeatures[m].id == gf->bufferFaceFeatures[n].id)
				{
					//intra class
					sumIntra += dist;
					numIntra++;
				}
				else
				{
					//extra class
					sumExtra += dist;
					numExtra++;
				}
			}
		}
		meanIntra = sumIntra / numIntra;
		meanExtra = sumExtra / numExtra;

		//Calculate variances
		for ( m = 0; m < numFaces; m++)
		{
			for ( n = m + 1; n < numFaces; n++)
			{
				dist = 0;
				for ( j = 0; j < histLength; j++)
				{
					//compute chi-square distance
					h1 = gf->bufferFaceFeatures[m].feature[ histLength * i + j];
					h2 = gf->bufferFaceFeatures[n].feature[ histLength * i + j];
					if ( h1 != 0 || h2 !=0)
					{
						tmpDist = h1 - h2;
						dist += tmpDist * tmpDist / ( h1 + h2);
					}
				}
				if (gf->bufferFaceFeatures[m].id == gf->bufferFaceFeatures[n].id)
				{
					//intra class
					sumIntraVar += (dist - meanIntra)*(dist - meanIntra);
				}
				else
				{
					//extra class
					sumExtraVar += (dist - meanExtra)*(dist - meanExtra);
				}
			}
		}
		varIntra = sumIntraVar / numIntra;
		varExtra = sumExtraVar / numExtra;

		//weight
		tmpWeight = (meanIntra - meanExtra) * (meanIntra - meanExtra) / ( varIntra + varExtra);
		for ( j = 0; j < histLength; j++)
		{
			weight[ i * histLength + j] = tmpWeight;
		}

	}//end region scan

	//write weights to file
	FILE * fp = fopen( WEIGHTS_BIN, "w+b");
	if(fp==NULL){
		printf("open file faces.bin failed!\n");fflush(stdout);
		exit(-1);
	}
	fwrite( weight, sizeof(float), histLength * numRegions, fp);
	fclose(fp);
		
	
}//end function trainWeight




void trainWeightForLBP(FACE3D_Type *gf)
{
	int numIntra, numExtra;
	float *weight = gf->histWeight;
	float tmpWeight;
	float sumIntra, sumExtra;
	float sumIntraVar, sumExtraVar;
	float meanIntra, meanExtra;
	float varIntra, varExtra;
	int i, j, m, n;
	//int numRegionH = gf->LGTCenters.numRegionH;
	//int numRegionW = gf->LGTCenters.numRegionW;
	int numRegions = TOTAL_FEATURE_LEN / NUM_BIN;	//num of regions
	int histLength = NUM_BIN;	//histogram length in each region
	int numFaces = gf->validFaces;
	float	dist,tmpDist, h1, h2;

	//float *tmpWeight = (float *)malloc(sizeof(float) * numRegions);

	for ( i = 0; i < numRegions; i++)
	{
		//for each region
		//Calculate means
		numIntra = 0;
		numExtra = 0;
		sumIntra = 0;
		sumExtra = 0;
		sumIntraVar = 0;
		sumExtraVar = 0;
		for ( m = 0; m < numFaces; m++)
		{
			for ( n = m + 1; n < numFaces; n++)
			{
				dist = 0;
				for ( j = 0; j < histLength; j++)
				{
					//compute chi-square distance
					h1 = gf->bufferFaceFeatures[m].feature[ histLength * i + j];
					h2 = gf->bufferFaceFeatures[n].feature[ histLength * i + j];
					if ( h1 != 0 || h2 !=0)
					{
						tmpDist = h1 - h2;
						dist += tmpDist * tmpDist / ( h1 + h2);
					}
				}
				if (gf->bufferFaceFeatures[m].id == gf->bufferFaceFeatures[n].id)
				{
					//intra class
					sumIntra += dist;
					numIntra++;
				}
				else
				{
					//extra class
					sumExtra += dist;
					numExtra++;
				}
			}
		}
		meanIntra = sumIntra / numIntra;
		meanExtra = sumExtra / numExtra;

		//Calculate variances
		for ( m = 0; m < numFaces; m++)
		{
			for ( n = m + 1; n < numFaces; n++)
			{
				dist = 0;
				for ( j = 0; j < histLength; j++)
				{
					//compute chi-square distance
					h1 = gf->bufferFaceFeatures[m].feature[ histLength * i + j];
					h2 = gf->bufferFaceFeatures[n].feature[ histLength * i + j];
					if ( h1 != 0 || h2 !=0)
					{
						tmpDist = h1 - h2;
						dist += tmpDist * tmpDist / ( h1 + h2);
					}
				}
				if (gf->bufferFaceFeatures[m].id == gf->bufferFaceFeatures[n].id)
				{
					//intra class
					sumIntraVar += (dist - meanIntra)*(dist - meanIntra);
				}
				else
				{
					//extra class
					sumExtraVar += (dist - meanExtra)*(dist - meanExtra);
				}
			}
		}
		varIntra = sumIntraVar / numIntra;
		varExtra = sumExtraVar / numExtra;

		//weight
		tmpWeight = (meanIntra - meanExtra) * (meanIntra - meanExtra) / ( varIntra + varExtra);
		for ( j = 0; j < histLength; j++)
		{
			weight[ i * histLength + j] = tmpWeight;
		}

	}//end region scan

	//write weights to file
	FILE * fp = fopen( WEIGHTS_BIN, "w+b");
	if(fp==NULL){
		printf("open file faces.bin failed!\n");fflush(stdout);
		exit(-1);
	}
	fwrite( weight, sizeof(float), histLength * numRegions, fp);
	fclose(fp);
		
	
}//end function trainWeight



void veriTrain( FACE3D_Type* gf)
{
	bool*	label;
	int		featLength = TOTAL_FEATURE_LEN;
	double**	featureDistance;
	unitFaceFeatClass* bufferFaceFeatures = gf->bufferFaceFeatures;
	int		numFaces = gf->validFaces;
	int		numIntra, numInter;

	int     groupSize = 3000;	//max group size
	int		numGroups;
	int		ratio = 3;	// inter/ intra ratio
	int		interRatio;  // skip rate for inter class pairs
	int		intraRatio = 1;
	int		i,j,k;
	int		numTotal;
	int		cnt, cntInter,cntIntra;
	float   h1, h2;

	numIntra = 0;
	interRatio = 0;
	for ( i = 0; i < numFaces; i++)
	{
		for ( j = i+1; j < numFaces; j++)
		{
			interRatio++;
			if( bufferFaceFeatures[i].id == bufferFaceFeatures[j].id)
			{
				numIntra++;
			}
		}
	}

	numIntra = (int) (numIntra / intraRatio);
	numInter = numIntra * ratio;
	numTotal = numIntra + numInter;
	interRatio = (int) ((float)interRatio / numInter); 
	numGroups = (int)ceil( (double)numTotal / groupSize);

	//allocate memory
	label = (bool*)malloc(groupSize * sizeof(bool));
	featureDistance = (double**)malloc( groupSize * sizeof(double*));
	for ( i = 0; i < numTotal; i++)
	{
		featureDistance[i] = (double*)malloc( TOTAL_FEATURE_LEN * sizeof(double));
	}


	cnt = 0;
	
	cntInter = 0;
	cntIntra = 0;
	
	//compute feature distance and store
	for ( i = 0; i < numFaces; i++)
	{
		for ( j = i + 1; j< numFaces; j++)
		{
			if ( cnt >= numTotal)
				break;

			if ( bufferFaceFeatures[i].id == bufferFaceFeatures[j].id)
			{
				//same class
				if ( cntIntra % intraRatio == 0)
				{
					for ( k = 0; k < TOTAL_FEATURE_LEN; k++)
					{
						h1 = bufferFaceFeatures[i].feature[k];
						h2 = bufferFaceFeatures[j].feature[k];
						featureDistance[cnt][k] = (h1 > h2) ?  ( h1 - h2): (h2 - h1);
						label[cnt] = 1;
					}
					cnt++;
				}
				cntIntra++;
			}
			else
			{
				//inter class
				if ( cntInter % interRatio == 0)
				{
					for ( k = 0; k < TOTAL_FEATURE_LEN; k++)
					{
						h1 = bufferFaceFeatures[i].feature[k];
						h2 = bufferFaceFeatures[j].feature[k];
						featureDistance[cnt][k] = (h1 > h2) ?  ( h1 - h2): (h2 - h1);
						label[cnt] = 0;
					}
					cnt++;
				}
				cntInter++;
			}
			
		}
	}
		
	//write to file
	FILE* pSVMTrain = fopen( SVM_TRAIN_BIN_FILE, "w+b");
	if ( pSVMTrain == NULL)
	{
		printf("error opening svmTrain.bin\n");
	}
	FILE* pSVMLabel = fopen( SVM_LABEL_BIN_FILE, "w+b");
	if ( pSVMLabel == NULL)
	{
		printf("error opening svmLabel.bin\n");
	}

	//label to file
	fwrite(&numTotal,sizeof(int), 1, pSVMLabel);
	fwrite(&featLength, sizeof(int),1, pSVMLabel);
	fwrite(label, sizeof(bool), numTotal, pSVMLabel);

	//feature distance to file
	for ( i = 0; i < numTotal; i++)
	{
		fwrite(featureDistance[i],sizeof(double), TOTAL_FEATURE_LEN, pSVMTrain);
	}

	fclose( pSVMTrain);
	fclose( pSVMLabel);







	//release memory
	if ( label != NULL)
	{
		free(label);
	}
	for ( i = 0; i < numTotal; i++)
	{
		if ( featureDistance[i] != NULL)
		{
			free(featureDistance[i]);
		}
	}
	if ( featureDistance != NULL)
	{
		free(featureDistance);
	}

	




}


void generateSVMList(FACE3D_Type* gf)
{
	svmPair* pairs;
	int		i, j, k;
	int		numGroups;
	int		numInGroup;
	int		tmpFaceNum = gf->fileList.listLength;
	int		validFaces;
	int		numIntra, numInter, numTotal;
	int		interSkip;
	int		count, countInter, countIntra, countSkip;
	int*	listIntra, *listInter;
	char	path[260];
	FILE*	pSVMOut;

	faceDetector * faceDet =  new faceDetector();
	

	//allocate temp memory
	char** tmpFileName = (char**)malloc( tmpFaceNum * sizeof(char*));
	for ( i = 0; i < tmpFaceNum; i++)
	{
		tmpFileName[i] = new char[260];
	}
	int* tmpID = (int*)malloc( tmpFaceNum * sizeof(int));

	validFaces = 0;
	for ( i = 0; i < tmpFaceNum; i++)
	{
		IplImage* imgFrame = NULL;
		imgFrame = cvLoadImage( gf->fileList.fileName[i], CV_LOAD_IMAGE_COLOR);
		if ( faceDet->runFaceDetector(imgFrame))
		{
			sprintf(tmpFileName[validFaces], "%s", gf->fileList.fileName[i]);
			tmpID[validFaces] = gf->fileList.fileID[i];
			validFaces++;
		}
	}
	gf->validFaces = validFaces;

	numIntra = 0;
	numTotal = 0;

	for ( i = 0; i < validFaces; i++)
	{
		for ( j = i + 1; j < validFaces; j++)
		{
			if ( tmpID[i] == tmpID[j])
			{
				numIntra++;
			}
			numTotal++;
		}
	}

	numInter = numIntra * INTER_INTRA_RATIO;	//limited number of intra pairs
	interSkip = (int) ((numTotal - numIntra) / numInter);
	numTotal = numInter + numIntra;
	numGroups = (int)ceil( (double)numTotal / PAIR_GROUP_SIZE);

	pairs = (svmPair*)malloc( numTotal * sizeof(svmPair));

	//randomize intra
	listIntra = (int*)malloc( numIntra * sizeof(int));
	for ( i = 0; i < numIntra; i++)
	{
		listIntra[i] = i;
	}
	shuffle( listIntra, numIntra);	//shuffle listIntra

	listInter = (int*)malloc( numInter * sizeof(int));
	for ( i = 0; i <numInter; i++)
	{
		listInter[i] = i + numIntra;
	}
	shuffle( listInter, numInter);	//shuffle inter

	//randomly assign pair groups
	countIntra = 0;
	countInter = numIntra;	//inter pairs starts from this position
	countSkip = 0;
	
	for ( i = 0; i < validFaces; i++)
	{
		for ( j = i+1; j < validFaces; j++)
		{
			if ( tmpID[i] == tmpID[j])
			{
				//intra class
				sprintf(pairs[countIntra].filename1,"%s", tmpFileName[i]);
				sprintf(pairs[countIntra].filename2,"%s", tmpFileName[j]);
				countIntra++;
			}
			else
			{
				//inter class
				if ( (countInter < numTotal) && (countSkip % interSkip == 0))
				{
					sprintf(pairs[countInter].filename1,"%s", tmpFileName[i]);
					sprintf(pairs[countInter].filename2,"%s", tmpFileName[j]);
					countInter++;
					countSkip = 0;
				}
				countSkip++;
			}
		}
	}

	
	//write to txt file
	for ( i = 0; i < numGroups - 1; i++)
	{
		sprintf(path, "%s%d.txt", SVM_LIST_DIR, i);
		pSVMOut = fopen(path, "w+");
		if ( pSVMOut == NULL)
		{
			printf("Error opening svm list txt file!\n");
		}

		if ( i == numGroups - 1)
		{
			continue; // throw out the following
			//last group may contain less than GROUP SIZE
			for ( j = PAIR_GROUP_INTRA * ( numGroups - 1); j < numIntra; j++)	//intra
			{
				fprintf(pSVMOut, "%d %s %s\n", 1, pairs[listIntra[j]].filename1, pairs[listIntra[j]].filename2);
			}
			for ( j = PAIR_GROUP_INTER * ( numGroups - 1); j < numInter; j++)
			{
				fprintf(pSVMOut, "%d %s %s\n", -1, pairs[listInter[j]].filename1, pairs[listInter[j]].filename2);
			}
		}
		else
		{
			for ( j = PAIR_GROUP_INTRA * i; j < PAIR_GROUP_INTRA * (i + 1); j++)
			{
				fprintf(pSVMOut, "%d %s %s\n", 1, pairs[listIntra[j]].filename1, pairs[listIntra[j]].filename2);
			}
			for ( j = PAIR_GROUP_INTER * i; j < PAIR_GROUP_INTER * (i + 1); j++)
			{
				fprintf(pSVMOut, "%d %s %s\n", -1, pairs[listInter[j]].filename1, pairs[listInter[j]].filename2);
			}
		}

		fclose(pSVMOut);
		pSVMOut = NULL;
	}








	


	//clean-ups
	if ( tmpID != NULL)
		free(tmpID);

	for ( i = 0; i < tmpFaceNum; i++)
	{
		if ( tmpFileName[i] != NULL)
		{
			delete [] tmpFileName[i];
		}
	}
	if (tmpFileName != NULL)
		free(tmpFileName);

	delete faceDet;

	if ( pairs != NULL)
		free(pairs);

	if ( listIntra != NULL)
		free(listIntra);

	if ( listInter != NULL)
		free(listInter);



}


void svmTrainFromList(int start, int end)
{
	FILE*	pList;
	FILE*	pFeature;
	char	path[260];
	char	fileName1[260], fileName2[260],tmpName[260];
	float	feature1[TOTAL_FEATURE_LEN], feature2[TOTAL_FEATURE_LEN], featureDist[TOTAL_FEATURE_LEN];
	float	tmpDist;
	IplImage *img1, *img2;
	int		label;
	int		countMod;
	int		i, j, k;
	int		tmpID;
	char*	pExtension;

	//eyesDetector * detectEye = new eyesDetector;
	//faceDetector * faceDet =  new faceDetector();

	assert( (end - start) % 2 == 0);

	//init svm global structure
	initSystem(&gst,&svm);
	gst.nSamples = PAIR_GROUP_SIZE;
	gst.featureSize = TOTAL_FEATURE_LEN;

	countMod = 0;
	for ( i = start; i <= end; i++)
	{
		sprintf(path, "%s%d.txt", SVM_LIST_DIR, i);
		pList = fopen(path, "r");
		if ( pList == NULL)
		{
			printf("Error loading svm list!\n");
		}
		for ( j = 0; j < PAIR_GROUP_SIZE; j++)
		{
			fscanf(pList, "%d %s %s\n", &label, fileName1, fileName2);
			
			//extract features
			printf("-----------------------------------------\n");
			puts(fileName1);
			puts(fileName2);
			printf("-----------------------------------------\n");

			//process filename
			strcpy(tmpName, fileName1);
			pExtension = strstr(tmpName, "/");
			while ( pExtension != NULL)
			{
				strcpy(tmpName, pExtension+1);
				pExtension = NULL;
				pExtension = strstr(tmpName, "/");

			}
			pExtension = strstr(tmpName, ".jpg");
			*pExtension = '\0';
			sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, tmpName);
			pFeature = fopen(path, "rb");
			fread(&tmpID, sizeof(int), 1, pFeature);
			fread(feature1, sizeof(float), TOTAL_FEATURE_LEN, pFeature);
			fclose(pFeature);

			strcpy(tmpName, fileName2);
			pExtension = strstr(tmpName, "/");
			while ( pExtension != NULL)
			{
				strcpy(tmpName, pExtension+1);
				pExtension = NULL;
				pExtension = strstr(tmpName, "/");

			}
			pExtension = strstr(tmpName, ".jpg");
			*pExtension = '\0';
			sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, tmpName);
			pFeature = fopen(path, "rb");
			fread(&tmpID, sizeof(int), 1, pFeature);
			fread(feature2, sizeof(float), TOTAL_FEATURE_LEN, pFeature);
			fclose(pFeature);

			//img1 = cvLoadImage(fileName1, CV_LOAD_IMAGE_COLOR);
			//img2 = cvLoadImage(fileName2, CV_LOAD_IMAGE_COLOR);

			//extractFeatureFromImage(gf, feature1, img1, detectEye, faceDet);
			//extractFeatureFromImage(gf, feature2, img2, detectEye, faceDet);

			//feature distance
			for (k = 0; k < TOTAL_FEATURE_LEN; k++)
			{
				tmpDist = feature1[k] - feature2[k];
				gst.features[j][k] = (tmpDist > 0)? tmpDist: ( 0 - tmpDist);
			}
			gst.classLable[j] = label;
			
			//cvReleaseImage(&img1);
			//cvReleaseImage(&img2);
		}

		//svm training
		sprintf(path, "%s%d.mod", SVM_LIST_DIR, countMod);
		svmTraining(gst.features, gst.nSamples, gst.featureSize, gst.classLable, path);
		countMod++;


		//delete detectEye;
		//delete faceDet;

		system("pause");

	}
	








}


void extractFeatureFromImage(FACE3D_Type* gf, float* feature, IplImage* pFrame, eyesDetector * detectEye, faceDetector * faceDet)
{
	int UL_x, UL_y;
	CvPoint pt1, pt2;
	//int size = sizeof( eyesDetector);
	//eyesDetector * detectEye = new eyesDetector;
	//faceDetector * faceDet =  new faceDetector();
	IplImage*  gray_face_CNN = cvCreateImage(cvSize(CNNFACECLIPHEIGHT,CNNFACECLIPWIDTH), 8, 1);

	CvPoint pointPos[6];
	CvPoint *leftEye    = &pointPos[0];
	CvPoint *rightEye   = &pointPos[2];
	CvPoint *leftMouth  = &pointPos[4];
	CvPoint *rightMouth = &pointPos[5];

	IplImage * tarImg = cvCreateImage( cvSize(warpedImgW, warpedImgH), IPL_DEPTH_8U, warpedImgChNum );


	if( faceDet->runFaceDetector(pFrame))
		{	
			/* there was a face detected by OpenCV. */
			
			IplImage * clonedImg = cvCloneImage(pFrame);

			detectEye->runEyeDetector(clonedImg, gray_face_CNN, faceDet, pointPos);

			cvReleaseImage(&clonedImg);

			UL_x = faceDet->faceInformation.LT.x;
			UL_y = faceDet->faceInformation.LT.y;

			// face width and height
			pt1.x =  faceDet->faceInformation.LT.x;
			pt1.y = faceDet->faceInformation.LT.y;
			pt2.x = pt1.x + faceDet->faceInformation.Width;
			pt2.y = pt1.y + faceDet->faceInformation.Height;

			faceRotate(leftEye, rightEye, pFrame, tarImg, faceDet->faceInformation.Width, faceDet->faceInformation.Height);

			grayDownsample(tarImg, gf, 0, FALSE);

			gf->featureLength = 0;

#if USE_CA
			extractCAFeature(&gf);
#endif

#if USE_GBP
			extractGBPFaceFeatures( (unsigned char*)(tarImg->imageData), (tarImg->widthStep), &gf);
#endif
#if USE_LBP
			extractLBPFaceFeatures( (unsigned char*)(tarImg->imageData), (tarImg->widthStep), gf, FALSE);
#endif
#if USE_GABOR
			extractGaborFeatures( &gf );
#endif

			memcpy( feature, gf->faceFeatures, TOTAL_FEATURE_LEN * sizeof(float));
	}
	

	//clean-ups
	cvReleaseImage( &gray_face_CNN);
	cvReleaseImage( &tarImg);
	//delete detectEye;
	//delete faceDet;

}


void prepareLBPFeaturesToFile( FACE3D_Type* gf)
{
	FILE*	pList;
	char	path[260];
	char	fileName[260];
	char*	pExtension;
	int		i, j;
	float	feature[TOTAL_FEATURE_LEN];

	eyesDetector * detectEye = new eyesDetector;
	faceDetector * faceDet =  new faceDetector();

	for ( i = 0; i < gf->fileList.listLength; i++)
	{
		IplImage* pFrame;
		pFrame = cvLoadImage(gf->fileList.fileName[i], CV_LOAD_IMAGE_COLOR);

		extractFeatureFromImage(gf, feature, pFrame, detectEye, faceDet);

		//write to file
		//remove extension
		strcpy(fileName, gf->fileList.fileName[i]);
		pExtension = strstr(fileName, "/");
		while ( pExtension != NULL)
		{
			strcpy(fileName, pExtension+1);
			pExtension = NULL;
			pExtension = strstr(fileName, "/");

		}
		pExtension = strstr(fileName, ".jpg");
		*pExtension = '\0';
		sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, fileName);
		pList = fopen(path, "w+b");
		if (pList == NULL)
		{
			printf("Error opening feature binary file %s!\n", path);
		}
		fwrite( &gf->fileList.fileID[i], sizeof(int), 1, pList);
		fwrite( feature, sizeof(float), TOTAL_FEATURE_LEN, pList);

		fclose(pList);
		cvReleaseImage(&pFrame);
	}

	delete detectEye;
	delete faceDet;

		
		


	



}


void boostSVMTrain()
{
	//config
	int		firstStep = 500;
	int		step = 500;
	int		maxTrain = 5000;
	int		listLength = 33000;
	int		maxStep = 10;

	//
	FILE*	pList, *pFeature;
	int		curTrain, curList;
	int		i, j, k;
	int		tmpID, label;
	char	path[260], fileName1[260], fileName2[260], tmpName[260];
	float	feature1[TOTAL_FEATURE_LEN], feature2[TOTAL_FEATURE_LEN];
	double	tmpDist;
	double	featDist[TOTAL_FEATURE_LEN];
	char*	pExtension;
	int		roundTrain;		// nth train step
	float	tmpScore;

	//init
	initSystem(&gst, &svm);
	gst.featureSize = TOTAL_FEATURE_LEN;

	sprintf(path, "%sfull.txt", SVM_LIST_DIR, i);
	pList = fopen(path, "r");
	if ( pList == NULL)
	{
		printf("Error loading svm list!\n");
	}


	//first train
	for ( i = 0; i < firstStep; i++)
	{
		fscanf(pList, "%d %s %s\n", &label, fileName1, fileName2);
			
			//extract features
			//printf("-----------------------------------------\n");
			//puts(fileName1);
			//puts(fileName2);
			//printf("-----------------------------------------\n");

			//process filename
			strcpy(tmpName, fileName1);
			pExtension = strstr(tmpName, "/");
			while ( pExtension != NULL)
			{
				strcpy(tmpName, pExtension+1);
				pExtension = NULL;
				pExtension = strstr(tmpName, "/");

			}
			pExtension = strstr(tmpName, ".jpg");
			*pExtension = '\0';
			sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, tmpName);
			pFeature = fopen(path, "rb");
			fread(&tmpID, sizeof(int), 1, pFeature);
			fread(feature1, sizeof(float), TOTAL_FEATURE_LEN, pFeature);
			fclose(pFeature);

			strcpy(tmpName, fileName2);
			pExtension = strstr(tmpName, "/");
			while ( pExtension != NULL)
			{
				strcpy(tmpName, pExtension+1);
				pExtension = NULL;
				pExtension = strstr(tmpName, "/");

			}
			pExtension = strstr(tmpName, ".jpg");
			*pExtension = '\0';
			sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, tmpName);
			pFeature = fopen(path, "rb");
			fread(&tmpID, sizeof(int), 1, pFeature);
			fread(feature2, sizeof(float), TOTAL_FEATURE_LEN, pFeature);
			fclose(pFeature);
			//feature dist
			for (k = 0; k < TOTAL_FEATURE_LEN; k++)
			{
				tmpDist = feature1[k] - feature2[k];
				gst.features[i][k] = (tmpDist > 0)? tmpDist: ( 0 - tmpDist);
			}
			gst.classLable[i] = label;
	}

	//svm training
	sprintf(path, "%sboost.mod", SVM_LIST_DIR);
	svmTraining(gst.features, firstStep, gst.featureSize, gst.classLable, path);
	//load mod for testing
	svm.svm_init_clean(path);

	curTrain = firstStep;
	curList = firstStep;

	//boosting train
	for ( i = curList; i < listLength; i++)
	{
		if ( curTrain >= (maxTrain - 1))
		{
			break;
		}
	
		fscanf(pList, "%d %s %s\n", &label, fileName1, fileName2);
		curList++;
		
		//extract features

		//process filename
		strcpy(tmpName, fileName1);
		pExtension = strstr(tmpName, "/");
		while ( pExtension != NULL)
		{
			strcpy(tmpName, pExtension+1);
			pExtension = NULL;
			pExtension = strstr(tmpName, "/");

		}
		pExtension = strstr(tmpName, ".jpg");
		*pExtension = '\0';
		sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, tmpName);
		pFeature = fopen(path, "rb");
		fread(&tmpID, sizeof(int), 1, pFeature);
		fread(feature1, sizeof(float), TOTAL_FEATURE_LEN, pFeature);
		fclose(pFeature);

		strcpy(tmpName, fileName2);
		pExtension = strstr(tmpName, "/");
		while ( pExtension != NULL)
		{
			strcpy(tmpName, pExtension+1);
			pExtension = NULL;
			pExtension = strstr(tmpName, "/");

		}
		pExtension = strstr(tmpName, ".jpg");
		*pExtension = '\0';
		sprintf(path, "%s%s.feat",LBP_FEATURE_DIR, tmpName);
		pFeature = fopen(path, "rb");
		fread(&tmpID, sizeof(int), 1, pFeature);
		fread(feature2, sizeof(float), TOTAL_FEATURE_LEN, pFeature);
		fclose(pFeature);
		//feature dist
		for (k = 0; k < TOTAL_FEATURE_LEN; k++)
		{
			tmpDist = feature1[k] - feature2[k];
			featDist[k] = (tmpDist > 0) ? tmpDist: (0 - tmpDist);
		}


		//test
		svmTest(featDist, TOTAL_FEATURE_LEN, &tmpScore, &svm);

		if ( tmpScore * label < 0)
		{
			//incorrect, add to train
			for (k = 0; k < TOTAL_FEATURE_LEN; k++)
			{
				gst.features[curTrain][k] = (float)featDist[k];
			}
			gst.classLable[curTrain] = label;
			curTrain++;
		}
		else
		{
			continue;
		}


		if ( (curTrain + 1) % step == 0)
		{
			//train again
			printf("%dth boost\n", (curTrain + 1) / step);
			printf("curren list: %d\n--------------------------\n", curList);
			svmTraining(gst.features, curTrain+1, gst.featureSize, gst.classLable, path);
			svm.svm_init_clean(path);
		}
	}

			


	
		


	system("pause");



}