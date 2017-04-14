// vehicle_col_model.cpp : 定义 DLL 应用程序的导出函数。
//
#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "vehicle_col_model.h"
#include "ColorData.h"
#include "./HaarAdaboost/HaarExport.h"
#include "./HaarAdaboost/BaseFunction.h"

#include "./inc/JWExport.h"
#include "./inc/SDKCommon.h"
#include "./TypeDetect/VehicleTypeDetect.h"

#include "opencv2/core/types_c.h"
#include "opencv2/opencv.hpp"
#pragma comment(lib, "LprSDK.lib")
#define debug

#define MAX_ELATASK_NUM     24   //任务最大支持数量

using namespace std;
using namespace cv;


OSAL_INT32 g_iHandleCount = 0;
OSAL_HANDLE  g_Handle[MAX_ELATASK_NUM] = {OSAL_NULL};

//OSAL_INT32 g_itypeHandleCount = 0;
//OSAL_HANDLE  g_typeHandle[MAX_ELATASK_NUM] = {OSAL_NULL};

string xml_path = "cascade_Haar_17_3.xml";
CascadeClassifier cascade;
SDKErrCode Vehicle_Init(OSAL_HANDLE *pHandle, int width, int height, pPlateRect pRect)

{

	cascade.load(xml_path);
	pTaskMain pTask = OSAL_NULL;

	OSAL_INT32 i = 0;
	OSAL_BOOL  bSuccess = OSAL_FALSE;

	if (g_iHandleCount > MAX_ELATASK_NUM )
	{
		return SDK_ERR_FULLTASK;
	}
	if (pHandle == OSAL_NULL)
	{
		return SDK_ERR_PARAM;
	}

	pRect->height = 0;
	pRect->width = 0;
	pRect->x = 0;
	pRect->y = 0;
	pRect->color = 0;

	pTask=(pTaskMain)malloc(sizeof(TaskMain));

	if (pTask == OSAL_NULL )
	{
		return SDK_ERR_ALLOC;
	}

	memset(pTask,0,sizeof(TaskMain));

	for( i=0; i < MAX_ELATASK_NUM; i++)
	{
		if( g_Handle[i] == OSAL_NULL)
		{
			pTask->m_iTaskIndex=i;
			g_Handle[i] = (OSAL_HANDLE)pTask;
			bSuccess=OSAL_TRUE;
			break;
		}
	}

	if(bSuccess==OSAL_TRUE)
	{
		JW_LprTaskCreate(&pTask->plate_model,width,height);
		pTask->adaboost_model=HaarAdaboostCreate(VEHICLE_BEFFACE_MODEL);
		//颜色
		//VehicleColor_TaskCreate(pTask,2);
		//memset(pTask->Colordata_image,0,sizeof(int)*3);

		
		//车型
		//pTask->data_image=(OSAL_INT32 *)malloc(756*sizeof(OSAL_INT32));
		pTask->nHist=(OSAL_INT32 *)malloc(2048*4*sizeof(OSAL_INT32));
		pTask->nTheta = (OSAL_INT32 *)malloc(2048 * 4 * sizeof(OSAL_INT32));
		pTask->nMeasure = (OSAL_INT32 *)malloc(2048 * 4 * sizeof(OSAL_INT32));
		memset(pTask->nHist, 0, sizeof(OSAL_INT32) * 2048 * 4);
		memset(pTask->nTheta, 0, sizeof(OSAL_INT32) * 2048 * 4);
		memset(pTask->nMeasure, 0, sizeof(OSAL_INT32) * 2048 * 4);
		memset(pTask->data_image,0,sizeof(OSAL_INT32)*3780);
		VehicleType_TaskCreate(pTask, 0);//////////////
		VehicleType_TaskCreate(pTask, 1);//////////////
		
		g_iHandleCount++;
		*pHandle=g_Handle[i];
		return SDK_ERR_NONE;
	}
	return SDK_ERR_OTHER;
}



void find2Max(int a[], int n, int&max1,int&max2)
{
	max1 = 0;
	max2 = 1;
	for (int i = 1; i<n; i++)
	{
		if (a[i] > a[max1])
		{
			max2 = max1;
			max1 = i;
		}
		else if (a[i] > a[max2] && a[i] < a[max1])
			max2 = i;
	}	
}

int  CalculateBGRForDetect(uchar*Img, int cols, int rows, float &rate,int v_color)
{
	int*ret = new int[cols*rows];
	int*color_hist = new int[colorclass];
	memset(ret, 0, sizeof(int)*cols*rows);
	memset(color_hist, 0, sizeof(int)*colorclass);

	for (int y = 0; y < rows; y++)
	{
		uchar*ptr = Img + 3 * y*cols;
		int*ptr_1 = ret + y*cols;
		for (int x = 0; x < cols; x++)
		{
			ptr_1[x] = getcolorclass((int)ptr[3 * x], (int)ptr[3 * x + 1], (int)ptr[3 * x + 2]);
		}
	}

	if (2==v_color)
	{
		for (int i = 0; i < cols*rows; i++)
		{
			if ((ret[i] != 1) && (ret[i] != 8))//黄牌打车剔除黑灰色；
			{
				color_hist[ret[i]]++;
			}		
		}
	}

	for (int i = 0; i < cols*rows; i++)
	{
		color_hist[ret[i]]++;
	}

	color_hist[2] += color_hist[6];//红粉合并

	int max1 = 0, max2 = 0;
	find2Max(color_hist, colorclass, max1, max2);//排序

	int color = -1;

	if ((max1 == 0) && ((max2 == 1) || (max2 == 8)))
	{
		color = 0;
	}

	if ((max1 == 1) && ((max2 == 0) || (max2 == 8)))
	{
		color = 1;
	}

	if ((max1 == 8) && ((max2 == 0) || (max2 == 1)))
	{
		color = 8;
	}

	if (((max1 == 0) || (max1 == 1) || (max1 == 8)) && ((max2 != 0) && (max2 != 1) && (max2 != 8)))
	{
		color = max2;
	}

	if ((max1 != 0) && (max1 != 1) && (max1 != 8))
	{
		color = max1;
	}

	rate = 0;
	if (color > -1)
	{
		rate = 1.0*color_hist[color] / (cols*rows);
		if (rate < 0.1)
		{
			color = max1;
		}
	}

	delete[]ret;
	delete[]color_hist;

	if (color == 8)
	{
		color = 1;
	}

	return color;
	
}


int MatResize(uchar*src, int col_s, int row_s, uchar*dst,int col_d,int row_d, float scale_x, float scale_y)
{
	uchar*buffer = new uchar[3*col_s*row_d];
	for (int y = 0; y < row_d; y++)
	{
		memcpy((buffer + 3 * y*col_s), (src + (int)(3 * y*scale_y)*col_s), sizeof(uchar) * 3 * col_s);//抽行
	}

	for (int y = 0; y < row_d; y++)
	{
		uchar*ptr  = buffer + 3 * y*col_s;
		uchar*ptr_d = dst + 3 * y*col_d;
		for (int x = 0; x < col_d; x++)
		{
			ptr_d[3 * x + 0] = ptr[(int)(3 * x*scale_x) + 0];
			ptr_d[3 * x + 1] = ptr[(int)(3 * x*scale_x) + 1];
			ptr_d[3 * x + 2] = ptr[(int)(3 * x*scale_x) + 2];
		}
	}
	
	delete[] buffer;
	return 0;
}

int i = 0;
char path1[256] = { 0 };
char path2[256] = { 0 };


SDKErrCode Vehicle_Process(OSAL_HANDLE handle, pColTypeImage image, ImageDataType IMAGE_TYPE_BGR, pPlateRect pRect, PTCResult Result)
{
	/************************************************************************/
	/*                      获取需要处理的数据                              */
	/************************************************************************/
	pTaskMain Handle = (pTaskMain)handle;

	IplImage*m_srcImage=OSAL_NULL;//存放源图
	m_srcImage = cvCreateImage(cvSize(image->width, image->height), image->depth, 3);
	m_srcImage->imageData = (OSAL_CHAR*)image->pData;
	Result->iVehicleColor = 10;

	/************************************************************************/
	/*                      变量声明区                                */
	/************************************************************************/
	OSAL_INT32 vehicles = 0;
	OSAL_INT32 ResultNum = 0;

	cv::Mat mat(m_srcImage);
	cv::Mat roiImage_color;//颜色识别区
	cv::Mat roiImage_type;//车型识别区；
	cv::Mat resizemat;//车脸检测用

	cv::Rect rect_face;//车脸坐标缓存；
	cv::Rect rect_color;//颜色识别区坐标
	cv::Rect rect_type;//车型判断区；
	
	THaarRect ppResult1;//蓝牌
	THaarRect ppResult2; //黄牌

	int v_color = 0  ;//颜色标识符，1 蓝牌，2黄牌
	VRECT MRect = { 0 };// 颜色定位区
	OSAL_INT32   iVehicleType = 5;//车型编号
	/************************************************************************/
	/*                      检测车脸                               */
	/************************************************************************/
	vector<Rect> objs;
	resize(mat, resizemat, Size(mat.cols / 8, mat.rows / 8));
	cvtColor(resizemat, resizemat, CV_BGR2GRAY);

	//imwrite("b.jpg", resizemat);
	cascade.detectMultiScale(resizemat, objs, 1.1, 1, 0
		                      //|CV_HAAR_FIND_BIGGEST_OBJECT   //3
		                      | CV_HAAR_DO_ROUGH_SEARCH     //2
		                      //CV_HAAR_SCALE_IMAGE       //1
		                      , cv::Size(20, 6));
	int vehiclenum = objs.size();
	/************************************************************************/
	/*                      一 有车牌输入                               */
	/************************************************************************/
	if (pRect->color)
	{
		/**********车牌在车脸内-找出最佳位置的车脸***********/
		if (vehiclenum)
		{
			vector<cv::Rect>::iterator it = objs.begin();
			rect_face = Rect(it->x, it->y, it->width, it->height);
			for (int j = 0; j < objs.size(); j++)
			{
				if ((it->x * 8 < pRect->x) && ((it->x + it->width) * 8 > (pRect->x + pRect->width)) &&
					((it->y + it->height / 2) * 8 < pRect->y) && (it->y + it->height) * 8 > (pRect->y + pRect->height))//车牌必须位于车脸的下半区内
				{
					int distance1 = (rect_face.y + rect_face.height) * 8 - (pRect->y + pRect->height) +
						abs((rect_face.x + rect_face.width / 2) * 8 - (pRect->x + pRect->width / 2));
					int distance2 = (it->y + it->height) * 8 - (pRect->y + pRect->height) +
						abs((it->x + it->width / 2) * 8 - (pRect->x + pRect->width / 2));
					if (distance2 < distance1)
					{
						rect_face = Rect(it->x, it->y, it->width, it->height);//求出与车牌最接近的车脸；
					}
				}
				it++;
			}
			rect_face.x = rect_face.x * 8;
			rect_face.y = rect_face.y * 8;
			rect_face.width = rect_face.width * 8;
			rect_face.height = rect_face.height * 8;
		}
		/**********1 蓝牌***********/
		if (pRect->color == 1)//1 蓝牌
		{
			v_color = 1;
		/**********1.1 颜色定位***********/
			MRect.bottom = pRect->y     - 2 * (pRect->height);
			MRect.top    = MRect.bottom - 3 * (pRect->height);
			MRect.left   = pRect->x - pRect->width;
			MRect.right  = pRect->x + pRect->width;

		/**********1.2 车脸定位***********/
	
			if (vehiclenum)//车牌位于车脸的下半区内
			{
				ppResult1.x = rect_face.x ;
				ppResult1.y = rect_face.y ;
				ppResult1.width  = rect_face.width  ;
				ppResult1.height = rect_face.height ;
			}
			else//无车脸
			{
				ppResult1.x = (pRect->x) - (pRect->width)*2.8;//防溢出；
				if (ppResult1.x < 0)//防溢出；
				{
					ppResult1.x = 0;
				}
				ppResult1.y = (pRect->y) - (pRect->height) * 6;
				if (ppResult1.y < 0)
				{
					ppResult1.y = 0;
				}
				ppResult1.width = (pRect->width)*5.3;
				ppResult1.height = (pRect->height)*10.7;
			}
		/**********1.3 车型判断***********/
			rect_type = cv::Rect(ppResult1.x, ppResult1.y, ppResult1.width, ppResult1.height);
			mat(rect_type).copyTo(roiImage_type);
			cvtColor(roiImage_type, roiImage_type, CV_BGR2GRAY);

			VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 30, &iVehicleType);
		}

		/**********2 黄牌***********/
		if (pRect->color == 2)  //2黄牌
		{
			v_color = 2;
		/**********2.1 颜色定位***********/
			MRect.bottom = pRect->y - 3 * (pRect->height);//比蓝牌高一点；
			MRect.top    = MRect.bottom - 3 * (pRect->height);
			MRect.left   = pRect->x - pRect->width;
			MRect.right  = pRect->x + pRect->width;

		/**********2.2 车脸定位***********/
			if (vehiclenum)//车牌位于车脸的下半区内
			{
				ppResult2.x = rect_face.x;
				ppResult2.y = rect_face.y;
				ppResult2.width  = rect_face.width  ;
				ppResult2.height = rect_face.height ;
			}
			else
			{
				ppResult2.x = (pRect->x) - (pRect->width)*3.5;
				ppResult2.y = (pRect->y) - (pRect->height)*9.8;
				ppResult2.width  = (pRect->width)*6.8;
				ppResult2.height = (pRect->height) * 13;
			}


		/**********2.3 车型判断***********/
			rect_type = cv::Rect(ppResult2.x, ppResult2.y, ppResult2.width, ppResult2.height);
			mat(rect_type).copyTo(roiImage_type);
			cvtColor(roiImage_type, roiImage_type, CV_BGR2GRAY);
			OSAL_INT32   iVehicleType = 5;
			VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 10, &iVehicleType);
		}

		/**********2 颜色识别***********/
		rect_color = cv::Rect(MRect.left, MRect.top, MRect.right - MRect.left, MRect.bottom - MRect.top);
		mat(rect_color).copyTo(roiImage_color);//划出颜色识别区；
		float      rate1 = 0;
		Result->iVehicleColor = CalculateBGRForDetect(roiImage_color.data, roiImage_color.cols, roiImage_color.rows, rate1, v_color);
	}
    
	/************************************************************************/
	/*                      二 无车牌输入                               */
	/************************************************************************/
	if (vehiclenum&&!pRect->color)
	{
		/**********1.确定车脸***********/
		rect_face = objs[0];
		for (int i = 0; i < vehiclenum; i++)
		{
			if (objs[i].y > rect_face.y)
			{
				rect_face = objs[i];//取最下面的一个框作为车脸；
			}
		}

		/**********2 划定车脸检测范围***********/
		rect_face.x = rect_face.x * 8;
		rect_face.y = rect_face.y * 8;
		rect_face.width  = rect_face.width  * 8;
		rect_face.height = rect_face.height * 8;

		/**********3 颜色识别区***********/
		MRect.top = rect_face.y + rect_face.height / 2 -15;
		MRect.bottom = MRect.top + 25;
		MRect.left = rect_face.x + rect_face.width / 2 - 40;
		MRect.right = rect_face.x + rect_face.width / 2 + 60;

		/**********4 车脸车型***********/
		rect_type = cv::Rect(rect_face.x, rect_face.y, rect_face.width, rect_face.height);
		mat(rect_type).copyTo(roiImage_type);
		VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 30, &iVehicleType);

		/**********5 车辆颜色***********/
		rect_color = cv::Rect(MRect.left, MRect.top, MRect.right - MRect.left, MRect.bottom - MRect.top);
		mat(rect_color).copyTo(roiImage_color);//划出颜色识别区；
		float      rate1 = 0;
		Result->iVehicleColor = CalculateBGRForDetect(roiImage_color.data, roiImage_color.cols, roiImage_color.rows, rate1, 1);
	}

#ifdef debug
	rectangle(mat, rect_color, cv::Scalar(0, 255, 0), 5);
	rectangle(mat, cv::Rect(pRect->x,pRect->y,pRect->width,pRect->height), cv::Scalar(255, 0, 255), 5);
	rectangle(mat, cv::Rect(rect_face.x , rect_face.y , rect_face.width, rect_face.height ), cv::Scalar(0, 0, 255), 5);
	imwrite("1.jpg", mat);
 	imwrite("2.jpg", roiImage_color);

	cv::namedWindow("Color_ROI", 0);
	imshow("Color_ROI", mat);
#endif

	//////////////////////////////////////////////////////////////////////////

	*pRect = { 0, 0, 0, 0, 0 };//复位
	Result->iVehicleType = iVehicleType;
	cvReleaseImage(&m_srcImage);
	 return SDK_ERR_NONE;

}

SDKErrCode Vehicle_Realse(OSAL_HANDLE *pHandle)
{
	pTaskMain handle=(pTaskMain)(*pHandle);

	int i=0;

	if(handle==	OSAL_NULL)
	{
		return SDK_ERR_PARAM;
	}

	if(handle->adaboost_model!=OSAL_NULL)
	{
		HaarAdaboostFree(handle->adaboost_model);
	}
	if(handle->plate_model!=OSAL_NULL)
	{
		JW_LprTaskFree(&handle->plate_model);
	}

	//颜色释放
	//if(handle->handle_color!=OSAL_NULL)
	//{
	//	VehicleColor_TaskFree(&handle->handle_color);
	//}

	//车型释放
	//if(handle->data_image!=OSAL_NULL)
	//{
	//	free(handle->data_image);
	//}
	if(handle->nHist!=OSAL_NULL)
	{
		free(handle->nHist);
	}
	if(handle->nMeasure !=OSAL_NULL)
	{
		free(handle->nMeasure);
	}
	if(handle->nTheta!=OSAL_NULL)
	{
		free(handle->nTheta);
	}
	if(handle->handle_yellow !=OSAL_NULL)
	{
		VehicleType_TaskFree(&handle->handle_yellow);
	}
	if(handle->handle_blue!=OSAL_NULL)
	{
		VehicleType_TaskFree(&handle->handle_blue);
	}

	i=handle->m_iTaskIndex;
	if(handle==g_Handle[i]&&g_Handle[i]!=OSAL_NULL)
	{
		free(g_Handle[i]);
		g_Handle[i]=OSAL_NULL;
		handle=OSAL_NULL;
		g_iHandleCount--;
	}
	else
	{
		return SDK_ERR_OTHER;
	}
	*pHandle = OSAL_NULL;
#ifdef DSPDM8127
	FREE_BIGMEM();
#endif 
	return SDK_ERR_NONE;
}

