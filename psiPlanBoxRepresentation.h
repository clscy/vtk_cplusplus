/*====================================================
ref vtkBoxRepresentation
=====================================================*/

#pragma once
#ifndef psiPlanBoxRepresentation_h
#define psiPlanBoxRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkPoints;
class vtkPlane;
class vtkSphereSource;
class vtkCellPicker;
class vtkTransform;
class vtkProperty;
class vtkPolyDataAlgorithm;
class vtkPointHandleRepresentation3D;
class vtkDoubleArray;
class vtkMatrix4x4;

class vtkLineSource;
class vtkTubeFilter;

class VTKINTERACTIONWIDGETS_EXPORT psiPlanBoxRepresentation : public vtkWidgetRepresentation
{
public:
	static psiPlanBoxRepresentation *New();
	vtkTypeMacro(psiPlanBoxRepresentation, vtkWidgetRepresentation);
	void PrintSelf(ostream& os, vtkIndent indent) override;

	vtkSetMacro(InsideOut,vtkTypeBool);
	vtkGetMacro(InsideOut, vtkTypeBool);
	vtkBooleanMacro(InsideOut, vtkTypeBool);

	void SetCylinderDiameter(double diameter);
	double GetCylinderDiameter();

	double *GetLinePoints();

	void GetBoxPolyData(vtkPolyData *pd);

	double* GetHexPoints();

	virtual void HandlesOn();
	virtual void HandlesOff();


	void PlaceWidget(double bounds[6]) override;
	void BuildRepresentation() override;
	int ComputeInteractionState(int X, int Y, int modify = 0) override;
	void StartWidgetInteraction(double e[2]) override;
	void WidgetInteraction(double e[2]) override;
	//double *GetBounds() VTK_SIZEHINT(6) override;
	void StartComplexInteraction(vtkRenderWindowInteractor *iren, vtkAbstractWidget *widget, unsigned long event, void *calldata) override;
	void ComplexInteraction(vtkRenderWindowInteractor *iren, vtkAbstractWidget *widget, unsigned long event, void *calldata) override;
	int ComputeComplexInteractionState(vtkRenderWindowInteractor *iren, vtkAbstractWidget *widget, unsigned long event, void *calldata, int modify = 0) override;
	void EndComplexInteraction(vtkRenderWindowInteractor *iren, vtkAbstractWidget *widget, unsigned long event, void *calldata) override;

	void ReleaseGraphicsResources(vtkWindow*) override;
	int RenderOpaqueGeometry(vtkViewport*) override;
	int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
	vtkTypeBool HasTranslucentPolygonalGeometry() override;

	enum {OutSide = 0,
		MoveF0,MoveF1,MoveF2,MoveF3,MoveF4,MoveF5,
		Translating,Rotating,Scaling,
		MoveL1P1, MoveL1P2, MoveL2P1, MoveL2P2,
		MoveL1,MoveL2
	};

	void SetInteractionState(int state);

	void StepForward();
	void StepBackward();

	void RegisterPickers() override;


protected:
	psiPlanBoxRepresentation();
	~psiPlanBoxRepresentation() override;

	// Manage how the representation appears
	double LastEventPosition[3];
	double LastEventOrientation[4];
	double StartEventOrientation[4];

	// the hexahedron (6 faces)
	vtkActor *HexActor;
	vtkPolyDataMapper *HexMapper;
	vtkPolyData *HexPolyData;
	vtkPoints *Points;
	double N[6][3];

	vtkActor *HexFace;
	vtkPolyDataMapper *HexFaceMapper;
	vtkPolyData *HexFacePolyData;

	// glyphs representing hot spots (e.g., handles)
	vtkActor          **Handle;
	vtkPolyDataMapper **HandleMapper;
	vtkSphereSource   **HandleGeometry;
	virtual void PositionHandles();
	int HighlightHandle(vtkProp *prop); //returns cell id
	void HighlightFace(int cellId);
	virtual void ComputeNormals();
	virtual void SizeHandles();

	void HighlightCylinder(vtkProp *prop);

	vtkActor *LinePointActor;
	vtkPolyDataMapper *LinePointMapper;
	vtkPolyData *LinePointPolyData;
	vtkPoints *LinePoints;

	vtkActor          **LinePointHandle;
	vtkPolyDataMapper **LinePointHandleMapper;
	vtkSphereSource   **LinePointHandleGeometry;

	// Do the picking
	vtkCellPicker *HandlePicker;
	vtkCellPicker *HexPicker;
	vtkActor *CurrentHandle;
	int      CurrentHexFace;
	vtkCellPicker *LastPicker;
	vtkCellPicker *LinePicker;

	// Transform the hexahedral points (used for rotations)
	vtkTransform *Transform;

	vtkProperty *HandleProperty;
	vtkProperty *SelectedHandleProperty;
	vtkProperty *FaceProperty;
	vtkProperty *SelectedFaceProperty;

	virtual void CreateDefaultProperties();

	vtkTypeBool InsideOut;

	// Helper methods
	virtual void Translate(double *p1, double *p2);
	virtual void Scale(double *p1, double *p2, int X, int Y);
	virtual void Rotate(int X, int Y, double *p1, double *p2, double *vpn);
	void MovePlusXFace(double *p1, double *p2);
	void MoveMinusXFace(double *p1, double *p2);
	void MovePlusYFace(double *p1, double *p2);
	void MoveMinusYFace(double *p1, double *p2);
	void MovePlusZFace(double *p1, double *p2);
	void MoveMinusZFace(double *p1, double *p2);
	void UpdatePose(double *p1, double *d1, double *p2, double *d2);

	void MoveLinePoint(double *p1, double *p2, int num);

	// Internal ivars for performance
	vtkPoints      *PlanePoints;
	vtkDoubleArray *PlaneNormals;
	vtkMatrix4x4   *Matrix;

	vtkPlane *Planes[6];

	//"dir" is the direction in which the face can be moved i.e. the axis passing
//through the center
	void MoveFace(double *p1, double *p2, double *dir,
		double *x1, double *x2, double *x3, double *x4,
		double *x5);
	//Helper method to obtain the direction in which the face is to be moved.
	//Handles special cases where some of the scale factors are 0.
	void GetDirection(const double Nx[3], const double Ny[3],
		const double Nz[3], double dir[3]);

	vtkLineSource* Line1Source;
	vtkTubeFilter *tubeFiler1;
	vtkPolyDataMapper* cylinderMapper1;
	vtkActor* CylinderActor1;

	vtkLineSource* Line2Source;
	vtkTubeFilter *tubeFiler2;
	vtkPolyDataMapper *cylinderMapper2;
	vtkActor* CylinderActor2;

	vtkProperty *CylinderProperty;
	vtkProperty *SelectedCylinderProperty;

	double radius = 1.5 / 2;



private:
	psiPlanBoxRepresentation(const psiPlanBoxRepresentation&) = delete;
	void operator=(const psiPlanBoxRepresentation&) = delete;

};

#endif