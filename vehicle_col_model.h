/****************************************************************************************
********
********   文件名:   vehicle_col_model.h
********   功能描述: 高速侧打车型及颜色对外接口函数声明
********   版本:     V1.0
********   日期:     2017-1-20
********  
*****************************************************************************************/
#pragma  once
#ifndef   COL_TYPE_EXPORT_H
#define   COL_TYPE_EXPORT_H

#define VEHICLE_COL_MODEL_EXPORTS
#ifdef VEHICLE_COL_MODEL_EXPORTS
#define VEHICLE_COL_MODEL_API __declspec(dllexport)
#else
#define VEHICLE_COL_MODEL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "./inc/OSAL_type.h"
#include "./inc/SDKCommon.h"
#include "./Vehiclecolor/Vehiclecolor.h"
#include "./TypeDetect/VehicleTypeDetect.h"


	typedef struct TagColTypeImage
	{
		OSAL_INT32 nSize;
		OSAL_INT32 nChannels;
		OSAL_INT32 depth;
		OSAL_INT32 step;
		OSAL_INT32 nColorMode;
		OSAL_INT32 dataOrder;
		OSAL_INT32 width;
		OSAL_INT32 height;
		OSAL_INT32 imageSize;
		OSAL_UCHAR *pData;	
	}ColTypeImage,*pColTypeImage;

//typedef enum
//{
//	Type_Truck                  = 0,//大卡车；
//	Type_Bus                   = 1,// 大客车
//	Type_Car                   = 2,// 小轿车
//	Type_MiniTruck             = 3,//小货车
//	Type_MiniBus               = 4,//面包车
//	Type_other                 = 5 // 其他
//
//} TYPEResult;
//typedef enum  Vehicle_COLOR
//{ 
//
//	V_White                   =0, //白色
//	V_Black                   =1, //黑色
//	V_Red                     =2,//红色
//	V_Yellow                  =3,//黄色
//	V_Blue                    =4,//蓝色
//	V_Green                   =5,//绿色
//	V_Pink                    =6,//粉色
//	V_Purple                  =7,//紫色
//	V_Gray                    =8,//灰色
//	V_Brown                   =9,//棕色
//	TypeColor_other           =10  //其他
//
//}COLOR_RESULT;
	typedef struct TagTaskMain
	{
		OSAL_HANDLE  adaboost_model;    //车脸定位句柄
		OSAL_HANDLE  plate_model;       //车牌识别句柄     

		//颜色
		OSAL_HANDLE  handle_color; //bp句柄
		OSAL_INT32 Colordata_image[3];

		//车型
		OSAL_HANDLE handle_blue;        //blue 颜色句柄
		OSAL_HANDLE handle_yellow;     //yellow 颜色句柄
		OSAL_INT32  data_image[3780];     //*hog;
		OSAL_INT32 *nMeasure;
		OSAL_INT32 *nTheta;
		OSAL_INT32 *nHist;

		OSAL_INT32   m_iTaskIndex;    //任务编号

	}TaskMain,*pTaskMain;


	//定义结果结构体
	typedef struct TagCResult
	{

		OSAL_INT32   iVehicleType;              /*  车辆类型 */
		OSAL_INT32   iVehicleColor;             /*  车身颜色 */

	}TCResult,*PTCResult;

	//车牌位置
	typedef struct PlateRect{
		OSAL_INT32   x;             
		OSAL_INT32   y;             
		OSAL_INT32   width;         
		OSAL_INT32   height;
		OSAL_INT32   color;//1 蓝牌；2 黄牌；3 其他; 0 不输入车牌信息；
	}*pPlateRect;




VEHICLE_COL_MODEL_API SDKErrCode Vehicle_Init(OSAL_HANDLE *pHandle, int width, int height, pPlateRect pRect);
VEHICLE_COL_MODEL_API SDKErrCode Vehicle_Process(OSAL_HANDLE handle, pColTypeImage image, ImageDataType eType, pPlateRect pRect, PTCResult Result);
VEHICLE_COL_MODEL_API SDKErrCode Vehicle_Realse(OSAL_HANDLE *pHandle);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif