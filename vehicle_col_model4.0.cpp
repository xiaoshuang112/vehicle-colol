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
//#define debug

#define MAX_ELATASK_NUM     24   //任务最大支持数量

using namespace std;


OSAL_INT32 g_iHandleCount = 0;
OSAL_HANDLE  g_Handle[MAX_ELATASK_NUM] = {OSAL_NULL};

//OSAL_INT32 g_itypeHandleCount = 0;
//OSAL_HANDLE  g_typeHandle[MAX_ELATASK_NUM] = {OSAL_NULL};

SDKErrCode Vehicle_Init(OSAL_HANDLE *pHandle, int width, int height, pPlateRect pRect)

{
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
	IplImage *GrayImage = OSAL_NULL,*ResizeImage=OSAL_NULL,*m_srcImage=OSAL_NULL;
	OSAL_INT32 vehicles = 0;
	PHaarWRect pHaarResult = OSAL_NULL;//车脸结果
	PJWLprResult pResult1 = OSAL_NULL;//车牌结果；
	OSAL_INT32 ResultNum = 0;
	cv::Mat roiImage_color;//颜色识别区
	cv::Mat roiImage_type;//车型识别区；

	pTaskMain Handle=(pTaskMain)handle;

	m_srcImage=cvCreateImage(cvSize(image->width,image->height),image->depth,3);
	m_srcImage->imageData=(OSAL_CHAR*)image->pData;
	

	cv::Mat mat(m_srcImage);
	ResizeImage = cvCreateImage(cvSize(m_srcImage->width/8,m_srcImage->height/8),m_srcImage->depth,1);
	GrayImage = cvCreateImage(cvSize(m_srcImage->width,m_srcImage->height),m_srcImage->depth,1);

	cvCvtColor(m_srcImage,GrayImage,CV_BGR2GRAY);
	cvResize(GrayImage,ResizeImage);

	THaarRect ppResult1;//蓝牌
	THaarRect ppResult2; //黄牌
	THaarRect ppResult;

	int v_color = 0  ;//1 蓝牌，2黄牌
	VRECT MRect = { 0 };// 颜色定位区
	OSAL_INT32   iVehicleType = 5;

	////////////////////////////有车牌输入////////////////////////////////////
	if (pRect->color)
	{
		if (Handle->adaboost_model)//车脸检测
		{
			vehicles = HaarDetectObjects(Handle->adaboost_model,(OSAL_PUCHAR)ResizeImage->imageData,ResizeImage->width,ResizeImage->height,
				       1.1, 3, HAAR_DO_ROUGH_SEARCH, 0, 0, 0, 0,&pHaarResult);
		}

		if (pRect->color == 1)//1 蓝牌
		{
			v_color = 1;
			//////////////////1.1 颜色定位区////////////////////////////////
			MRect.bottom = pRect->y     - 2 * (pRect->height);
			MRect.top    = MRect.bottom - 3 * (pRect->height);
			MRect.left   = pRect->x - pRect->width;
			MRect.right  = pRect->x + pRect->width;

			//////////////////构造车脸//////////////////////////////////

			//if (vehicles)
			//{
			//}

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

			////////////////车型判断////////////////////////////////////
			cv::Rect rect_type(ppResult1.x, ppResult1.y, ppResult1.width, ppResult1.height);
			mat(rect_type).copyTo(roiImage_type);
			cvtColor(roiImage_type, roiImage_type, CV_BGR2GRAY);

			VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 30, &iVehicleType);
		}
		if (pRect->color == 2)  //2黄牌
		{
			v_color = 2;

			//////////////////2.1颜色定位区////////////////////////////////
			MRect.bottom = pRect->y - 2 * (pRect->height);
			MRect.top    = MRect.bottom - 3 * (pRect->height);
			MRect.left   = pRect->x - pRect->width;
			MRect.right  = pRect->x + pRect->width;

			//////////////////构造车脸//////////////////////////////////
			ppResult2.x = (pRect->x) - (pRect->width)*3.5;
			ppResult2.y = (pRect->y) - (pRect->height)*9.8;
			ppResult2.width = (pRect->width)*6.8;
			ppResult2.height = (pRect->height) * 13;

			////////////////车型判断////////////////////////////////////
			cv::Rect rect_type(ppResult1.x, ppResult1.y, ppResult1.width, ppResult1.height);
			mat(rect_type).copyTo(roiImage_type);
			cvtColor(roiImage_type, roiImage_type, CV_BGR2GRAY);
			OSAL_INT32   iVehicleType = 5;
			VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 30, &iVehicleType);
		}

		//////////////////////颜色识别//////////////////////////////////
		cv::Rect rect_color(MRect.left, MRect.top, MRect.right - MRect.left, MRect.bottom - MRect.top);
		mat(rect_color).copyTo(roiImage_color);//划出颜色识别区；
		float      rate1 = 0;
		Result->iVehicleColor = CalculateBGRForDetect(roiImage_color.data, roiImage_color.cols, roiImage_color.rows, rate1, v_color);
		rectangle(mat, rect_color, cv::Scalar(0, 0, 255), 5);
		imwrite("1.jpg", mat);
		imwrite("2.jpg", roiImage_color);

	}
    
	/////////////////////////////无车牌输入///////////////////////////////////////
	/**********1.检测车脸***********/
	if (!pRect->color)
	{
		if (Handle->adaboost_model)
		{
			vehicles = HaarDetectObjects(Handle->adaboost_model,
				(OSAL_PUCHAR)ResizeImage->imageData,
				ResizeImage->width,
				ResizeImage->height,
				1.1,
				3,
				HAAR_DO_ROUGH_SEARCH,
				0,
				0,
				0,
				0,
				&pHaarResult);
		}

		/**********2.划定车牌检测范围***********/
		if (vehicles > 0) //
		{
			//printf("Face Detect OK\n");
			pHaarResult->x *= 8;
			pHaarResult->y *= 8;
			pHaarResult->width *= 8;
			pHaarResult->height *= 8;

			TJWRect lprRect;
			lprRect.left = pHaarResult->x;
			lprRect.right = pHaarResult->x + pHaarResult->width;
			lprRect.top = pHaarResult->y;
			lprRect.bottom = pHaarResult->y + pHaarResult->height;

			JW_SetParam(Handle->plate_model, LPR_Set_DetectROI, &lprRect);

		}
		else
		{
			TJWRect lprRoi;
			lprRoi.left = 0;
			lprRoi.right = image->width;
			lprRoi.top = image->height * 0.25;
			lprRoi.bottom = image->height;

			JW_SetParam(Handle->plate_model, LPR_Set_DetectROI, &lprRoi);
		}

		/**********3.车牌检测***********/
		int iret = JW_PlateRecog(Handle->plate_model, (OSAL_PUCHAR)m_srcImage->imageData,
			m_srcImage->width, m_srcImage->height, IMAGE_TYPE_BGR, &pResult1, &ResultNum);

		/**********4.有车牌***********/
		if (ResultNum > 0 && pResult1->CreditPlate > 900000)
		{
			/**********4.1 划定颜色检测区***********/
			MRect.bottom = pResult1->iYStart - 2 * (pResult1->iYEnd - pResult1->iYStart);
			MRect.top    = MRect.bottom      - 3 * (pResult1->iYEnd - pResult1->iYStart);
			MRect.left   = pResult1->iXStart    -  (pResult1->iXEnd - pResult1->iXStart);
			MRect.right  = pResult1->iXEnd;

			cv::Rect rect_color(MRect.left, MRect.top, MRect.right - MRect.left, MRect.bottom - MRect.top);
			mat(rect_color).copyTo(roiImage_color);

			/**********4.2 划定蓝牌车脸检测区,车型判断***********/
			if (pResult1->PlateColor == 30)//蓝牌
			{
				v_color = 1;

				/**********4.2.1 划定车脸***********/
				ppResult1.x = (pResult1->iXStart) - (pResult1->iXEnd - pResult1->iXStart) * 2.8;
				ppResult1.y = (pResult1->iYStart) - (pResult1->iYEnd - pResult1->iYStart) * 6;
				ppResult1.width  = (pResult1->iXEnd - pResult1->iXStart) * 5.3;
				ppResult1.height = (pResult1->iYEnd - pResult1->iYStart) * 10.7;

				/**********4.2.2 车型判断***********/
				if (ppResult1.x > 0 && ppResult1.y > 0 && ((ppResult1.x + ppResult1.width) < image->width) && ((ppResult1.y + ppResult1.height) < image->height))
				{
					cv::Rect rect_type(ppResult1.x, ppResult1.y, ppResult1.width, ppResult1.height);
					mat(rect_type).copyTo(roiImage_type);
					cvtColor(roiImage_type, roiImage_type, CV_BGR2GRAY);
					
					VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 30, &iVehicleType);
				}
			}

			/**********4.3 划定黄牌车脸检测区,车型判断***********/

			if (pResult1->PlateColor == 10)//黄牌
			{
				/**********4.3.1 划定车脸***********/
				v_color = 2;
				ppResult2.x = (pResult1->iXStart) - (pResult1->iXEnd - pResult1->iXStart) * 3.5;
				ppResult2.y = (pResult1->iYStart) - (pResult1->iYEnd - pResult1->iYStart) * 9.8;
				ppResult2.width = (pResult1->iXEnd - pResult1->iXStart) * 6.8;
				ppResult2.height = (pResult1->iYEnd - pResult1->iYStart) * 13;

				/**********4.3.2 车型判断***********/
				if (ppResult2.x > 0 && ppResult2.y > 0 && (ppResult2.x + ppResult2.width) < image->width && 
					(ppResult2.y + ppResult2.height) < image->height)
				{
					cv::Rect rect_rect(ppResult2.x, ppResult2.y, ppResult2.width, ppResult2.height);
					mat(rect_rect).copyTo(roiImage_color);
					VehicleType_TaskProcess(Handle, roiImage_color.data, roiImage_color.cols, roiImage_color.rows, IMAGE_TYPE_BGR, 10, &iVehicleType);
				}
			}

			/**********4.4 颜色检测***********/
			cv::Rect rect_color1(MRect.left, MRect.top, MRect.right - MRect.left, MRect.bottom - MRect.top);
			mat(rect_color1).copyTo(roiImage_color);
			float      rate1 = 0;
			Result->iVehicleColor = CalculateBGRForDetect(roiImage_color.data, roiImage_color.cols, roiImage_color.rows, rate1, v_color);

			rectangle(mat, rect_color1,cv::Scalar(0,0,255),5);
			imwrite("1.jpg", mat);
		}
		/**********5无车牌***********/
		else
		{
			/**********5.1 颜色识别区***********/
			MRect.top = pHaarResult->y + pHaarResult->height / 3;
			MRect.bottom = MRect.top + 20;
			MRect.left = pHaarResult->x + pHaarResult->width / 2 - 40;
			MRect.right = pHaarResult->x + pHaarResult->width / 2 + 60;

			/**********5.2 车脸车型***********/
			cv::Rect rect_type1(pHaarResult->x, pHaarResult->y, pHaarResult->width, pHaarResult->height);
			mat(rect_type1).copyTo(roiImage_type);
			VehicleType_TaskProcess(Handle, roiImage_type.data, roiImage_type.cols, roiImage_type.rows, IMAGE_TYPE_BGR, 30, &iVehicleType);

			/**********5.3 车辆颜色***********/
			float      rate1 = 0;
			Result->iVehicleColor = CalculateBGRForDetect(roiImage_color.data, roiImage_color.cols, roiImage_color.rows, rate1, 1);
		}	
	}

	//////////////////////////////////////////////////////////////////////////

	
#ifdef debug

	   cv::Scalar fd(0, 0, 255);
	   cv::Rect rect_haar(pHaarResult->x, pHaarResult->y, pHaarResult->width, pHaarResult->height);
	   cv::Rect rect_jw(pResult1->iXStart, pResult1->iYStart, pResult1->iXEnd - pResult1->iXStart, pResult1->iYEnd - pResult1->iYStart);

	   rectangle(mat, rect_color, fd, 2);
	   rectangle(mat, rect_haar,  fd, 2);
	   rectangle(mat, rect_jw,    fd, 2);

	   sprintf(path1, "%s%d.jpg", "E:\\1-work\\1-work-space\\3月\\testdata\\车脸定位\\", i);
	   sprintf(path2, "%s%d.jpg", "E:\\1-work\\1-work-space\\3月\\testdata\\颜色定位\\", i);

	   cv::imwrite(path1, mat);
	   cv::imwrite(path2, roiImage_color);

	   i++;
#endif



	Result->iVehicleType = iVehicleType;
	cvReleaseImage(&ResizeImage);
	cvReleaseImage(&GrayImage);
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

