
#include "psiPlanBoxRepresentation.h"

#include <vtkObjectFactory.h>

#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkPoints.h"
#include "vtkSphereSource.h"
#include "vtkCellPicker.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"
#include "vtkProperty.h"
#include "vtkEventData.h"
#include "vtkCamera.h"
#include "vtkInteractorObserver.h"
#include "vtkRenderer.h"
#include "vtkQuaternion.h"
#include "vtkMath.h"
#include "vtkVectorOperators.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderWindow.h"
#include "vtkPickingManager.h"

#include "vtkLineSource.h"
#include "vtkTubeFilter.h"

vtkStandardNewMacro(psiPlanBoxRepresentation);

psiPlanBoxRepresentation::psiPlanBoxRepresentation()
{
	this->InteractionState = psiPlanBoxRepresentation::OutSide;

	this->HandleSize = 5.0;

	this->InsideOut = 0;

	for (int i = 0; i < 6; ++i)
	{
		this->Planes[i] = vtkPlane::New();
	}

	this->CreateDefaultProperties();

	this->HexPolyData = vtkPolyData::New();
	this->HexMapper = vtkPolyDataMapper::New();
	this->HexMapper->SetInputData(this->HexPolyData);
	this->HexActor = vtkActor::New();
	this->HexActor->SetMapper(this->HexMapper);
	//outlineProperty

	this->Points = vtkPoints::New(VTK_DOUBLE);
	this->Points -> SetNumberOfPoints(15); //8corners; 6 faces; 1center
	this->HexPolyData->SetPoints(this->Points);

	//Construct connectivity for the faces. These are used to perform the picking.
	int i;
	vtkIdType pts[4];
	vtkCellArray *cells = vtkCellArray::New();
	cells->Allocate(cells->EstimateSize(6,4));
	pts[0] = 3; pts[1] = 0; pts[2] = 4; pts[3] = 7;//��˳����ת�����ַ��򣬱�֤��ķ�������
	cells->InsertNextCell(4,pts);
	pts[0] = 1; pts[1] = 2; pts[2] = 6; pts[3] = 5;
	cells->InsertNextCell(4, pts);
	pts[0] = 0; pts[1] = 1; pts[2] = 5; pts[3] = 4;
	cells->InsertNextCell(4, pts);
	pts[0] = 2; pts[1] = 3; pts[2] = 7; pts[3] = 6;
	cells->InsertNextCell(4, pts);
	pts[0] = 0; pts[1] = 3; pts[2] = 2; pts[3] = 1;
	cells->InsertNextCell(4, pts);
	pts[0] = 4; pts[1] = 5; pts[2] = 6; pts[3] = 7;
	cells->InsertNextCell(4, pts);
	this->HexPolyData->SetPolys(cells);
	cells->Delete();
	this->HexPolyData->BuildCells();

	//The face of the hexahedra
	cells = vtkCellArray::New();
	cells->Allocate(cells->EstimateSize(1, 4));
	cells->InsertNextCell(4,pts); // temporary,replaced later
	this->HexFacePolyData = vtkPolyData::New();
	this->HexFacePolyData->SetPoints(this->Points);
	this->HexFacePolyData->SetPolys(cells);
	this->HexFaceMapper = vtkPolyDataMapper::New();
	this->HexFaceMapper->SetInputData(this->HexFacePolyData);
	this->HexFace = vtkActor::New();
	this->HexFace->SetMapper(this->HexFaceMapper);
	this->HexFace->SetProperty(this->FaceProperty);
	cells->Delete();

	// Create the handles
	this->Handle = new vtkActor*[7];
	this->HandleMapper = new vtkPolyDataMapper*[7];
	this->HandleGeometry = new vtkSphereSource*[7];
	for (i = 0; i < 7; i++)
	{
		this->HandleGeometry[i] = vtkSphereSource::New();
		this->HandleGeometry[i]->SetThetaResolution(16);
		this->HandleGeometry[i]->SetPhiResolution(8);
		this->HandleMapper[i] = vtkPolyDataMapper::New();
		this->HandleMapper[i]->SetInputConnection(
			this->HandleGeometry[i]->GetOutputPort());
		this->Handle[i] = vtkActor::New();
		this->Handle[i]->SetMapper(this->HandleMapper[i]);
		this->Handle[i]->SetProperty(this->HandleProperty);
	}

	this->LinePointPolyData = vtkPolyData::New();
	this->LinePointMapper = vtkPolyDataMapper::New();
	this->LinePointMapper->SetInputData(this->LinePointPolyData);
	this->LinePointActor = vtkActor::New();
	this->LinePointActor->SetMapper(this->LinePointMapper);

	this->LinePoints = vtkPoints::New(VTK_DOUBLE);
	this->LinePoints->SetNumberOfPoints(4); 
	this->LinePointPolyData->SetPoints(this->LinePoints);

	// Represent the line
	this->Line1Source = vtkLineSource::New();
	this->Line1Source->SetResolution(5);
	this->Line1Source->SetPoint1(this->LinePoints->GetPoint(0));
	this->Line1Source->SetPoint2(this->LinePoints->GetPoint(1));
	tubeFiler1 = vtkTubeFilter::New();
	tubeFiler1->SetInputConnection(this->Line1Source->GetOutputPort());
	tubeFiler1->SetRadius(this->radius);
	tubeFiler1->SetNumberOfSides(50);
	tubeFiler1->CappingOn();
	cylinderMapper1 = vtkPolyDataMapper::New();
	cylinderMapper1->SetInputConnection(tubeFiler1->GetOutputPort());
	this->CylinderActor1 = vtkActor::New();
	this->CylinderActor1->SetMapper(cylinderMapper1);
	this->CylinderActor1->SetProperty(this->CylinderProperty);

	this->Line2Source = vtkLineSource::New();
	this->Line2Source->SetResolution(5);
	this->Line2Source->SetPoint1(this->LinePoints->GetPoint(2));
	this->Line2Source->SetPoint2(this->LinePoints->GetPoint(3));
	tubeFiler2 = vtkTubeFilter::New();
	tubeFiler2->SetInputConnection(this->Line2Source->GetOutputPort());
	tubeFiler2->SetRadius(this->radius);
	tubeFiler2->SetNumberOfSides(50);
	tubeFiler2->CappingOn();
	cylinderMapper2 = vtkPolyDataMapper::New();
	cylinderMapper2->SetInputConnection(tubeFiler2->GetOutputPort());
	this->CylinderActor2 = vtkActor::New();
	this->CylinderActor2->SetMapper(cylinderMapper2);
	this->CylinderActor2->SetProperty(this->CylinderProperty);

	this->LinePointHandle = new vtkActor*[4];
	this->LinePointHandleMapper = new vtkPolyDataMapper*[4];
	this->LinePointHandleGeometry = new vtkSphereSource*[4];
	for (i = 0; i < 4; i++)
	{
		this->LinePointHandleGeometry[i] = vtkSphereSource::New();
		this->LinePointHandleGeometry[i]->SetThetaResolution(16);
		this->LinePointHandleGeometry[i]->SetPhiResolution(8);
		this->LinePointHandleMapper[i] = vtkPolyDataMapper::New();
		this->LinePointHandleMapper[i]->SetInputConnection(
			this->LinePointHandleGeometry[i]->GetOutputPort());
		this->LinePointHandle[i] = vtkActor::New();
		this->LinePointHandle[i]->SetMapper(this->LinePointHandleMapper[i]);
		this->LinePointHandle[i]->SetProperty(this->HandleProperty);
	}

	// Define the point coordinates
	double bounds[6];
	bounds[0] = -1.5;
	bounds[1] = 1.5;
	bounds[2] = -1.5;
	bounds[3] = 1.5;
	bounds[4] = -1.5;
	bounds[5] = 1.5;
	this->PlaceWidget(bounds);

	//Manage the picking stuff
	this->HandlePicker = vtkCellPicker::New();
	this->HandlePicker->SetTolerance(0.001);
	for (i = 0; i < 7; i++)
	{
		this->HandlePicker->AddPickList(this->Handle[i]);
	}
	for (i = 0; i < 4; i++)
	{
		this->HandlePicker->AddPickList(this->LinePointHandle[i]);
	}
	this->HandlePicker->PickFromListOn();

	this->LinePicker = vtkCellPicker::New();
	this->LinePicker->SetTolerance(0.01);
	this->LinePicker->AddPickList(this->CylinderActor1);
	this->LinePicker->AddPickList(this->CylinderActor2);
	this->LinePicker->PickFromListOn();

	this->HexPicker = vtkCellPicker::New();
	this->HexPicker->SetTolerance(0.001);
	this->HexPicker->AddPickList(HexActor);
	this->HexPicker->PickFromListOn();

	this->CurrentHandle = nullptr;

	// Internal data members for performance
	this->Transform = vtkTransform::New();
	this->PlanePoints = vtkPoints::New(VTK_DOUBLE);
	this->PlanePoints->SetNumberOfPoints(6);
	this->PlaneNormals = vtkDoubleArray::New();
	this->PlaneNormals->SetNumberOfComponents(3);
	this->PlaneNormals->SetNumberOfTuples(6);
	this->Matrix = vtkMatrix4x4::New();

}

psiPlanBoxRepresentation::~psiPlanBoxRepresentation()
{
	this->HexActor->Delete();
	this->HexMapper->Delete();
	this->HexPolyData->Delete();
	this->Points->Delete();

	this->HexFace->Delete();
	this->HexFaceMapper->Delete();
	this->HexFacePolyData->Delete();

	for (int i = 0; i < 7; i++)
	{
		this->HandleGeometry[i]->Delete();
		this->HandleMapper[i]->Delete();
		this->Handle[i]->Delete();
	}
	delete[] this->Handle;
	delete[] this->HandleMapper;
	delete[] this->HandleGeometry;

	for (int i = 0; i < 4; ++i)
	{
		this->LinePointHandleGeometry[i]->Delete();
		this->LinePointHandleMapper[i]->Delete();
		this->LinePointHandle[i]->Delete();
	}
	delete[] this->LinePointHandle;
	delete[] this->LinePointHandleMapper;
	delete[] this->LinePointHandleGeometry;

	this->LinePoints->Delete();
	this->Line1Source->Delete();
	this->CylinderActor1->Delete();
	this->Line2Source->Delete();
	this->CylinderActor2->Delete();
	tubeFiler1->Delete();
	cylinderMapper1->Delete();
	tubeFiler2->Delete();
	cylinderMapper2->Delete();

	this->CylinderProperty->Delete();
	this->SelectedCylinderProperty->Delete();

	this->HandlePicker->Delete();
	this->HexPicker->Delete();
	this->LinePicker->Delete();

	this->Transform->Delete();
	this->PlanePoints->Delete();
	this->PlaneNormals->Delete();
	this->Matrix->Delete();

	this->HandleProperty->Delete();
	this->SelectedHandleProperty->Delete();
	this->FaceProperty->Delete();
	this->SelectedFaceProperty->Delete();

	for (int i = 0; i < 6; ++i)
	{
		this->Planes[i]->Delete();
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::CreateDefaultProperties()
{
	// Handle properties
	this->HandleProperty = vtkProperty::New();
	this->HandleProperty->SetColor(1, 1, 1);

	this->SelectedHandleProperty = vtkProperty::New();
	this->SelectedHandleProperty->SetColor(1, 0, 0);

	// Face properties
	this->FaceProperty = vtkProperty::New();
	this->FaceProperty->SetColor(1, 1, 1);
	//this->FaceProperty->SetOpacity(0.0);

	this->SelectedFaceProperty = vtkProperty::New();
	this->SelectedFaceProperty->SetColor(1, 1, 0);
	//this->SelectedFaceProperty->SetOpacity(0.25);

	this->CylinderProperty = vtkProperty::New();
	this->CylinderProperty->SetColor(1,1,1);
	//this->CylinderProperty->SetOpacity(0.0);

	this->SelectedCylinderProperty = vtkProperty::New();
	this->SelectedCylinderProperty->SetColor(1, 1, 0);
	//this->SelectedCylinderProperty->SetOpacity(0.0);
}

//----------------------------------------------------------------------
void psiPlanBoxRepresentation::GetBoxPolyData(vtkPolyData *pd)
{
	pd->SetPoints(this->HexPolyData->GetPoints());
	pd->SetPolys(this->HexPolyData->GetPolys());
}

//----------------------------------------------------------------------
double* psiPlanBoxRepresentation::GetHexPoints()
{
	return  static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
}

double *psiPlanBoxRepresentation::GetLinePoints()
{
	return  static_cast<vtkDoubleArray *>(this->LinePoints->GetData())->GetPointer(0);
}

double psiPlanBoxRepresentation::GetCylinderDiameter()
{
	return this->radius*2.0;
}

//----------------------------------------------------------------------
void psiPlanBoxRepresentation::StartWidgetInteraction(double e[2])
{
	// Store the start position
	this->StartEventPosition[0] = e[0];
	this->StartEventPosition[1] = e[1];
	this->StartEventPosition[2] = 0.0;

	// Store the start position
	this->LastEventPosition[0] = e[0];
	this->LastEventPosition[1] = e[1];
	this->LastEventPosition[2] = 0.0;

	this->ComputeInteractionState(static_cast<int>(e[0]), static_cast<int>(e[1]), 0);
}

void psiPlanBoxRepresentation::StartComplexInteraction(
	vtkRenderWindowInteractor *,
	vtkAbstractWidget *,
	unsigned long, void *calldata)
{
	vtkEventData *edata = static_cast<vtkEventData *>(calldata);
	vtkEventDataDevice3D *edd = edata->GetAsEventDataDevice3D();
	if (edd)
	{
		edd->GetWorldPosition(this->StartEventPosition);
		this->LastEventPosition[0] = this->StartEventPosition[0];
		this->LastEventPosition[1] = this->StartEventPosition[1];
		this->LastEventPosition[2] = this->StartEventPosition[2];
		edd->GetWorldOrientation(this->StartEventOrientation);
		std::copy(this->StartEventOrientation, this->StartEventOrientation + 4,
			this->LastEventOrientation);
	}
}


//----------------------------------------------------------------------
void psiPlanBoxRepresentation::WidgetInteraction(double e[2])
{
	// Convert events to appropriate coordinate systems
	vtkCamera *camera = this->Renderer->GetActiveCamera();
	if (!camera)
	{
		return;
	}
	double focalPoint[4], pickPoint[4], prevPickPoint[4];
	double z, vpn[3];
	camera->GetViewPlaneNormal(vpn);

	// Compute the two points defining the motion vector
	double pos[3];
	if (this->LastPicker == this->HexPicker)
	{
		this->HexPicker->GetPickPosition(pos);
	}
	else if(this->LastPicker == this->HandlePicker)
	{
		this->HandlePicker->GetPickPosition(pos);
	}
	else if (this->LastPicker == this->LinePicker)
	{
		this->LinePicker->GetPickPosition(pos);
	}

	vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
		pos[0], pos[1], pos[2],
		focalPoint);
	z = focalPoint[2];
	vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, this->LastEventPosition[0],
		this->LastEventPosition[1], z, prevPickPoint);
	vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

	// Process the motion
	if (this->InteractionState == psiPlanBoxRepresentation::MoveF0)
	{
		this->MoveMinusXFace(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::MoveF1)
	{
		this->MovePlusXFace(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::MoveF2)
	{
		this->MoveMinusYFace(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::MoveF3)
	{
		this->MovePlusYFace(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::MoveF4)
	{
		this->MoveMinusZFace(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::MoveF5)
	{
		this->MovePlusZFace(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::Translating)
	{
		this->Translate(prevPickPoint, pickPoint);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::Scaling)
	{
		this->Scale(prevPickPoint, pickPoint,
			static_cast<int>(e[0]), static_cast<int>(e[1]));
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::Rotating)
	{
		this->Rotate(static_cast<int>(e[0]), static_cast<int>(e[1]), prevPickPoint, pickPoint, vpn);
	}

	else if (this->InteractionState == psiPlanBoxRepresentation::MoveL1P1)
	{
		this->MoveLinePoint(prevPickPoint, pickPoint, 0);
	}
	else if (this->InteractionState == psiPlanBoxRepresentation::MoveL1P2)
	{
		this->MoveLinePoint(prevPickPoint, pickPoint, 1);
	}
	else if (this->InteractionState == psiPlanBoxRepresentation::MoveL2P1)
	{
		this->MoveLinePoint(prevPickPoint, pickPoint, 2);
	}
	else if (this->InteractionState == psiPlanBoxRepresentation::MoveL2P2)
	{
		this->MoveLinePoint(prevPickPoint, pickPoint, 3);
	}
	else if (this->InteractionState == psiPlanBoxRepresentation::MoveL1)
	{
		this->MoveLinePoint(prevPickPoint, pickPoint, 4);
	}
	else if (this->InteractionState == psiPlanBoxRepresentation::MoveL2)
	{
		this->MoveLinePoint(prevPickPoint, pickPoint, 5);
	}

	// Store the start position
	this->LastEventPosition[0] = e[0];
	this->LastEventPosition[1] = e[1];
	this->LastEventPosition[2] = 0.0;
}

void psiPlanBoxRepresentation::ComplexInteraction(
	vtkRenderWindowInteractor *,
	vtkAbstractWidget *,
	unsigned long, void *calldata)
{
	vtkEventData *edata = static_cast<vtkEventData *>(calldata);
	vtkEventDataDevice3D *edd = edata->GetAsEventDataDevice3D();
	if (edd)
	{
		// all others
		double eventPos[3];
		edd->GetWorldPosition(eventPos);
		double eventDir[4];
		edd->GetWorldOrientation(eventDir);

		double *prevPickPoint = this->LastEventPosition;
		double *pickPoint = eventPos;

		if (this->InteractionState == psiPlanBoxRepresentation::MoveF0)
		{
			this->MoveMinusXFace(prevPickPoint, pickPoint);
		}

		else if (this->InteractionState == psiPlanBoxRepresentation::MoveF1)
		{
			this->MovePlusXFace(prevPickPoint, pickPoint);
		}

		else if (this->InteractionState == psiPlanBoxRepresentation::MoveF2)
		{
			this->MoveMinusYFace(prevPickPoint, pickPoint);
		}

		else if (this->InteractionState == psiPlanBoxRepresentation::MoveF3)
		{
			this->MovePlusYFace(prevPickPoint, pickPoint);
		}

		else if (this->InteractionState == psiPlanBoxRepresentation::MoveF4)
		{
			this->MoveMinusZFace(prevPickPoint, pickPoint);
		}

		else if (this->InteractionState == psiPlanBoxRepresentation::MoveF5)
		{
			this->MovePlusZFace(prevPickPoint, pickPoint);
		}

		else if (this->InteractionState == psiPlanBoxRepresentation::Translating)
		{
			this->UpdatePose(this->LastEventPosition, this->LastEventOrientation,
				eventPos, eventDir);
		}

		// Book keeping
		std::copy(eventPos, eventPos + 3, this->LastEventPosition);
		std::copy(eventDir, eventDir + 4, this->LastEventOrientation);
		this->Modified();
	}
}

void psiPlanBoxRepresentation::StepForward()
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	this->Translate(pts, pts + 3);
}

void psiPlanBoxRepresentation::StepBackward()
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	this->Translate(pts + 3, pts);
}

void psiPlanBoxRepresentation::EndComplexInteraction(
	vtkRenderWindowInteractor *,
	vtkAbstractWidget *,
	unsigned long, void *)
{
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MoveFace(double *p1, double *p2, double *dir,
	double *x1, double *x2, double *x3, double *x4,
	double *x5)
{
	int i;
	double v[3], v2[3];

	for (i = 0; i < 3; i++)
	{
		v[i] = p2[i] - p1[i];
		v2[i] = dir[i];
	}

	vtkMath::Normalize(v2);
	double f = vtkMath::Dot(v, v2);

	for (i = 0; i < 3; i++)
	{
		v[i] = f * v2[i];

		x1[i] += v[i];
		x2[i] += v[i];
		x3[i] += v[i];
		x4[i] += v[i];
		x5[i] += v[i];
	}
	this->PositionHandles();
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::GetDirection(const double Nx[3], const double Ny[3],
	const double Nz[3], double dir[3])
{
	double dotNy, dotNz;
	double y[3];

	if (vtkMath::Dot(Nx, Nx) != 0)
	{
		dir[0] = Nx[0];
		dir[1] = Nx[1];
		dir[2] = Nx[2];
	}
	else
	{
		dotNy = vtkMath::Dot(Ny, Ny);
		dotNz = vtkMath::Dot(Nz, Nz);
		if (dotNy != 0 && dotNz != 0)
		{
			vtkMath::Cross(Ny, Nz, dir);
		}
		else if (dotNy != 0)
		{
			//dir must have been initialized to the
			//corresponding coordinate direction before calling
			//this method
			vtkMath::Cross(Ny, dir, y);
			vtkMath::Cross(y, Ny, dir);
		}
		else if (dotNz != 0)
		{
			//dir must have been initialized to the
			//corresponding coordinate direction before calling
			//this method
			vtkMath::Cross(Nz, dir, y);
			vtkMath::Cross(y, Nz, dir);
		}
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MovePlusXFace(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

	double *h1 = pts + 3 * 9;

	double *x1 = pts + 3 * 1;
	double *x2 = pts + 3 * 2;
	double *x3 = pts + 3 * 5;
	double *x4 = pts + 3 * 6;

	double dir[3] = { 1 , 0 , 0 };
	this->ComputeNormals();
	this->GetDirection(this->N[1], this->N[3], this->N[5], dir);
	this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MoveMinusXFace(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

	double *h1 = pts + 3 * 8;

	double *x1 = pts + 3 * 0;
	double *x2 = pts + 3 * 3;
	double *x3 = pts + 3 * 4;
	double *x4 = pts + 3 * 7;

	double dir[3] = { -1,0,0 };
	this->ComputeNormals();
	this->GetDirection(this->N[0], this->N[4], this->N[2], dir);

	this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MovePlusYFace(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

	double *h1 = pts + 3 * 11;

	double *x1 = pts + 3 * 2;
	double *x2 = pts + 3 * 3;
	double *x3 = pts + 3 * 6;
	double *x4 = pts + 3 * 7;

	double dir[3] = { 0,1,0 };
	this->ComputeNormals();
	this->GetDirection(this->N[3], this->N[5], this->N[1], dir);

	this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MoveMinusYFace(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

	double *h1 = pts + 3 * 10;

	double *x1 = pts + 3 * 0;
	double *x2 = pts + 3 * 1;
	double *x3 = pts + 3 * 4;
	double *x4 = pts + 3 * 5;

	double dir[3] = { 0, -1, 0 };
	this->ComputeNormals();
	this->GetDirection(this->N[2], this->N[0], this->N[4], dir);

	this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MovePlusZFace(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

	double *h1 = pts + 3 * 13;

	double *x1 = pts + 3 * 4;
	double *x2 = pts + 3 * 5;
	double *x3 = pts + 3 * 6;
	double *x4 = pts + 3 * 7;

	double dir[3] = { 0,0,1 };
	this->ComputeNormals();
	this->GetDirection(this->N[5], this->N[1], this->N[3], dir);

	this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::MoveMinusZFace(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

	double *h1 = pts + 3 * 12;

	double *x1 = pts + 3 * 0;
	double *x2 = pts + 3 * 1;
	double *x3 = pts + 3 * 2;
	double *x4 = pts + 3 * 3;

	double dir[3] = { 0,0,-1 };
	this->ComputeNormals();
	this->GetDirection(this->N[4], this->N[2], this->N[0], dir);

	this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

void psiPlanBoxRepresentation::MoveLinePoint(double *p1, double *p2,int num)
{
	double *pts = static_cast<vtkDoubleArray *>(this->LinePoints->GetData())->GetPointer(0);
	double v[3];

	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	//// Move the points
	//for (int i = 0; i < 4; i++)
	//{
	//	*pts++ += v[0];
	//	*pts++ += v[1];
	//	*pts++ += v[2];
	//}

	if (num < 4)
	{
		(pts + 3 * num)[0] += v[0];
		(pts + 3 * num)[1] += v[1];
		(pts + 3 * num)[2] += v[2];
	}
	else if (num == 4)
	{
		for (int i = 0; i < 2; i++)
		{
			*pts++ += v[0];
			*pts++ += v[1];
			*pts++ += v[2];
		}
	}
	else if (num == 5)
	{
		for (int i = 2; i < 4; i++)
		{
			(pts + 3 * i)[0] += v[0];
			(pts + 3 * i)[1] += v[1];
			(pts + 3 * i)[2] += v[2];
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		this->LinePointHandleGeometry[i]->SetCenter(this->LinePoints->GetPoint(i));
	}

	this->LinePoints->GetData()->Modified();
	this->LinePointPolyData->Modified();

	this->Line1Source->SetPoint1(this->LinePoints->GetPoint(0));
	this->Line1Source->SetPoint2(this->LinePoints->GetPoint(1));
	this->Line1Source->Update();

	this->Line2Source->SetPoint1(this->LinePoints->GetPoint(2));
	this->Line2Source->SetPoint2(this->LinePoints->GetPoint(3));
	this->Line2Source->Update();
}


//----------------------------------------------------------------------------
// Loop through all points and translate them
void psiPlanBoxRepresentation::Translate(double *p1, double *p2)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	double v[3];

	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	// Move the corners
	for (int i = 0; i < 8; i++)
	{
		*pts++ += v[0];
		*pts++ += v[1];
		*pts++ += v[2];
	}

	double *linePts = static_cast<vtkDoubleArray *>(this->LinePoints->GetData())->GetPointer(0);
	for (int i = 0; i < 4; ++i)
	{
		*linePts++ += v[0];
		*linePts++ += v[1];
		*linePts++ += v[2];
	}
	this->PositionHandles();
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::Scale(double *vtkNotUsed(p1),
	double *vtkNotUsed(p2),
	int vtkNotUsed(X),
	int Y)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	double *center
		= static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(3 * 14);
	double sf;

	if (Y > this->LastEventPosition[1])
	{
		sf = 1.03;
	}
	else
	{
		sf = 0.97;
	}

	// Move the corners
	for (int i = 0; i < 8; i++, pts += 3)
	{
		pts[0] = sf * (pts[0] - center[0]) + center[0];
		pts[1] = sf * (pts[1] - center[1]) + center[1];
		pts[2] = sf * (pts[2] - center[2]) + center[2];
	}

	double *linepts =
		static_cast<vtkDoubleArray *>(this->LinePoints->GetData())->GetPointer(0);
	for (int i = 0; i < 4; i++,linepts += 3)
	{
		linepts[0] = sf * (linepts[0] - center[0]) + center[0];
		linepts[1] = sf * (linepts[1] - center[1]) + center[1];
		linepts[2] = sf * (linepts[2] - center[2]) + center[2];
	}
	this->PositionHandles();
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::ComputeNormals()
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	double *p0 = pts;
	double *px = pts + 3 * 1;
	double *py = pts + 3 * 3;
	double *pz = pts + 3 * 4;
	int i;

	for (i = 0; i < 3; i++)
	{
		this->N[0][i] = p0[i] - px[i];
		this->N[2][i] = p0[i] - py[i];
		this->N[4][i] = p0[i] - pz[i];
	}
	vtkMath::Normalize(this->N[0]);
	vtkMath::Normalize(this->N[2]);
	vtkMath::Normalize(this->N[4]);
	for (i = 0; i < 3; i++)
	{
		this->N[1][i] = -this->N[0][i];
		this->N[3][i] = -this->N[2][i];
		this->N[5][i] = -this->N[4][i];
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::Rotate(int X,
	int Y,
	double *p1,
	double *p2,
	double *vpn)
{
	double *pts =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	double *center =
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(3 * 14);
	double v[3]; //vector of motion
	double axis[3]; //axis of rotation
	double theta; //rotation angle
	int i;

	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	// Create axis of rotation and angle of rotation
	vtkMath::Cross(vpn, v, axis);
	if (vtkMath::Normalize(axis) == 0.0)
	{
		return;
	}
	int *size = this->Renderer->GetSize();
	double l2 = (X - this->LastEventPosition[0])*(X - this->LastEventPosition[0])
		+ (Y - this->LastEventPosition[1])*(Y - this->LastEventPosition[1]);
	theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));

	//Manipulate the transform to reflect the rotation
	this->Transform->Identity();
	this->Transform->Translate(center[0], center[1], center[2]);
	this->Transform->RotateWXYZ(theta, axis);
	this->Transform->Translate(-center[0], -center[1], -center[2]);

	//Set the corners
	vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
	this->Transform->TransformPoints(this->Points, newPts);

	for (i = 0; i < 8; i++, pts += 3)
	{
		this->Points->SetPoint(i, newPts->GetPoint(i));
	}

	newPts->Delete();

	newPts = vtkPoints::New(VTK_DOUBLE);
	this->Transform->TransformPoints(this->LinePoints, newPts);

	for (i = 0; i < 4; i++, pts += 3)
	{
		this->LinePoints->SetPoint(i, newPts->GetPoint(i));
	}

	this->PositionHandles();
}

void psiPlanBoxRepresentation::UpdatePose(
	double *pos1, double *orient1,
	double *pos2, double *orient2
)
{
	vtkVector3d basis[3];
	double basisSize[3];

	vtkQuaternion<double> q2;
	q2.SetRotationAngleAndAxis(
		vtkMath::RadiansFromDegrees(orient2[0]), orient2[1], orient2[2], orient2[3]);

	for (int i = 0; i < 3; ++i)
	{
		// compute the net rotation
		vtkQuaternion<double> q1;
		
		{
			q1.SetRotationAngleAndAxis(
				vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
		}
		q1.Conjugate();
		vtkQuaternion<double> q3 = q2 * q1;
		double axis[4];
		axis[0] = vtkMath::DegreesFromRadians(q3.GetRotationAngleAndAxis(axis + 1));

		//Manipulate the transform to reflect the rotation
		this->Transform->Identity();
		this->Transform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);

		//Set the corners
		vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
		this->Transform->TransformPoints(this->Points, newPts);

		vtkVector3d p0(newPts->GetPoint(0));
		vtkVector3d p1(newPts->GetPoint((i > 0 ? i + 2 : 1)));
		basis[i] = p1 - p0;
		basisSize[i] = 0.5*basis[i].Normalize();

		newPts->Delete();
	}


	// get the translation
	vtkVector3d trans;
	for (int i = 0; i < 3; i++)
	{
		trans[i] = pos2[i] - pos1[i];
	}

	vtkQuaternion<double> q1;
	q1.SetRotationAngleAndAxis(
		vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
	q1.Conjugate();
	vtkQuaternion<double> q3 = q2 * q1;
	double axis[4];
	axis[0] = vtkMath::DegreesFromRadians(q3.GetRotationAngleAndAxis(axis + 1));

	// compute the new center based on the rotation
	// point of rotation and translation
	vtkVector3d center(
		static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(3 * 14));

	this->Transform->Identity();
	this->Transform->Translate(pos1[0], pos1[1], pos1[2]);
	this->Transform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);
	this->Transform->Translate(-(pos1[0]), -(pos1[1]), -(pos1[2]));
	this->Transform->Translate(center[0], center[1], center[2]);

	this->Transform->GetPosition(center.GetData());
	center = center + trans;

	// rebuild points based on basis vectors
	this->Points->SetPoint(0, (center - basis[0] * basisSize[0] - basis[1] * basisSize[1] - basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(1, (center + basis[0] * basisSize[0] - basis[1] * basisSize[1] - basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(2, (center + basis[0] * basisSize[0] + basis[1] * basisSize[1] - basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(3, (center - basis[0] * basisSize[0] + basis[1] * basisSize[1] - basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(4, (center - basis[0] * basisSize[0] - basis[1] * basisSize[1] + basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(5, (center + basis[0] * basisSize[0] - basis[1] * basisSize[1] + basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(6, (center + basis[0] * basisSize[0] + basis[1] * basisSize[1] + basis[2] * basisSize[2]).GetData());
	this->Points->SetPoint(7, (center - basis[0] * basisSize[0] + basis[1] * basisSize[1] + basis[2] * basisSize[2]).GetData());

	this->PositionHandles();
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::PlaceWidget(double bds[6])
{
	int i;
	double bounds[6], center[3];

	this->AdjustBounds(bds, bounds, center);

	this->Points->SetPoint(0, bounds[0], bounds[2], bounds[4]);
	this->Points->SetPoint(1, bounds[1], bounds[2], bounds[4]);
	this->Points->SetPoint(2, bounds[1], bounds[3], bounds[4]);
	this->Points->SetPoint(3, bounds[0], bounds[3], bounds[4]);
	this->Points->SetPoint(4, bounds[0], bounds[2], bounds[5]);
	this->Points->SetPoint(5, bounds[1], bounds[2], bounds[5]);
	this->Points->SetPoint(6, bounds[1], bounds[3], bounds[5]);
	this->Points->SetPoint(7, bounds[0], bounds[3], bounds[5]);

	double len = 10.0;
	double pt1L1[3],pt2L1[3],pt1L2[3],pt2L2[3];
	pt1L1[0] = (bounds[0] + bounds[1]) / 2.0;
	pt1L1[1] = bounds[2] - len;
	pt1L1[2] = bounds[4];

	pt2L1[0] = (bounds[0] + bounds[1]) / 2.0;
	pt2L1[1] = bounds[2] + (bounds[3] - bounds[2]) / 4.0;
	pt2L1[2] = bounds[5];

	pt1L2[0] = (bounds[0] + bounds[1]) / 2.0;
	pt1L2[1] = bounds[3] + len;
	pt1L2[2] = bounds[4];

	pt2L2[0] = (bounds[0] + bounds[1]) / 2.0;
	pt2L2[1] = bounds[3] - (bounds[3] - bounds[2]) / 4.0;
	pt2L2[2] = bounds[5];

	double v1[3], v2[3];
	for (int i = 0; i < 3; ++i)
	{
		v1[i] = pt2L1[i] - pt1L1[i];
		v2[i] = pt2L2[i] - pt1L2[i];
	}
	vtkMath::Normalize(v1);
	vtkMath::Normalize(v2);

	this->LinePoints->SetPoint(0, pt1L1);
	this->LinePoints->SetPoint(1, pt2L1[0]+v1[0]*len, pt2L1[1] + v1[1] * len, pt2L1[2] + v1[2] * len);
	this->LinePoints->SetPoint(2, pt1L2);
	this->LinePoints->SetPoint(3, pt2L2[0]+v2[0]*len, pt2L2[1] +v2[1] * len, pt2L2[2] + v2[2] * len);

	for (i = 0; i < 6; i++)
	{
		this->InitialBounds[i] = bounds[i];
	}
	this->InitialLength = sqrt((bounds[1] - bounds[0])*(bounds[1] - bounds[0]) +
		(bounds[3] - bounds[2])*(bounds[3] - bounds[2]) +
		(bounds[5] - bounds[4])*(bounds[5] - bounds[4]));

	this->PositionHandles();
	this->ComputeNormals();
	this->ValidPick = 1; //since we have set up widget
	this->SizeHandles();
}

//----------------------------------------------------------------------------
int psiPlanBoxRepresentation::ComputeInteractionState(int X, int Y, int modify)
{
	// Okay, we can process this. Try to pick handles first;
	// if no handles picked, then pick the bounding box.
	if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
	{
		this->InteractionState = psiPlanBoxRepresentation::OutSide;
		return this->InteractionState;
	}

	// Try and pick a handle first
	this->LastPicker = nullptr;
	this->CurrentHandle = nullptr;

	vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

	if (path != nullptr)
	{
		this->ValidPick = 1;
		this->LastPicker = this->HandlePicker;
		this->CurrentHandle = reinterpret_cast<vtkActor *>(path->GetFirstNode()->GetViewProp());
		if (this->CurrentHandle == this->Handle[0])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveF0;
		}
		else if (this->CurrentHandle == this->Handle[1])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveF1;
		}
		else if (this->CurrentHandle == this->Handle[2])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveF2;
		}
		else if (this->CurrentHandle == this->Handle[3])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveF3;
		}
		else if (this->CurrentHandle == this->Handle[4])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveF4;
		}
		else if (this->CurrentHandle == this->Handle[5])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveF5;
		}
		else if (this->CurrentHandle == this->Handle[6])
		{
			this->InteractionState = psiPlanBoxRepresentation::Translating;
		}
		else if (this->CurrentHandle == this->LinePointHandle[0])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveL1P1;
		}
		else if (this->CurrentHandle == this->LinePointHandle[1])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveL1P2;
		}
		else if (this->CurrentHandle == this->LinePointHandle[2])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveL2P1;
		}
		else if (this->CurrentHandle == this->LinePointHandle[3])
		{
			this->InteractionState = psiPlanBoxRepresentation::MoveL2P2;
		}

	}
	else //see if the hex is picked
	{
		path = this->GetAssemblyPath(X, Y, 0., this->HexPicker);

		if (path != nullptr)
		{
			this->LastPicker = this->HexPicker;
			this->ValidPick = 1;
			if (!modify)
			{
				this->InteractionState = psiPlanBoxRepresentation::Rotating;
			}
			else
			{
				this->CurrentHandle = this->Handle[6];
				this->InteractionState = psiPlanBoxRepresentation::Translating;
			}
		}
		else
		{

			path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

			if (path != nullptr)
			{
				this->LastPicker = this->LinePicker;
				this->ValidPick = 1;

				this->CurrentHandle = reinterpret_cast<vtkActor *>(path->GetFirstNode()->GetViewProp());
				if (this->CurrentHandle == this->CylinderActor1)
				{
					this->InteractionState = psiPlanBoxRepresentation::MoveL1;
				}
				else if (this->CurrentHandle == this->CylinderActor2)
				{
					this->InteractionState = psiPlanBoxRepresentation::MoveL2;
				}
			}
			else
			{
				this->InteractionState = psiPlanBoxRepresentation::OutSide;
			}
		}
	}

	return this->InteractionState;
}

int psiPlanBoxRepresentation::ComputeComplexInteractionState(
	vtkRenderWindowInteractor *,
	vtkAbstractWidget *,
	unsigned long, void *calldata, int)
{
	this->InteractionState = psiPlanBoxRepresentation::OutSide;

	vtkEventData *edata = static_cast<vtkEventData *>(calldata);
	vtkEventDataDevice3D *edd = edata->GetAsEventDataDevice3D();
	if (edd)
	{
		double pos[3];
		edd->GetWorldPosition(pos);

		// Try and pick a handle first
		this->LastPicker = nullptr;
		this->CurrentHandle = nullptr;

		vtkAssemblyPath* path = this->GetAssemblyPath3DPoint(pos, this->HandlePicker);

		if (path != nullptr)
		{
			this->ValidPick = 1;
			this->LastPicker = this->HandlePicker;
			this->CurrentHandle = reinterpret_cast<vtkActor *>(path->GetFirstNode()->GetViewProp());
			if (this->CurrentHandle == this->Handle[0])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveF0;
			}
			else if (this->CurrentHandle == this->Handle[1])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveF1;
			}
			else if (this->CurrentHandle == this->Handle[2])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveF2;
			}
			else if (this->CurrentHandle == this->Handle[3])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveF3;
			}
			else if (this->CurrentHandle == this->Handle[4])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveF4;
			}
			else if (this->CurrentHandle == this->Handle[5])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveF5;
			}
			else if (this->CurrentHandle == this->Handle[6])
			{
				this->InteractionState = psiPlanBoxRepresentation::Translating;
			}
			else if (this->CurrentHandle == this->LinePointHandle[0])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveL1P1;
			}
			else if (this->CurrentHandle == this->LinePointHandle[1])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveL1P2;
			}
			else if (this->CurrentHandle == this->LinePointHandle[2])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveL2P1;
			}
			else if (this->CurrentHandle == this->LinePointHandle[3])
			{
				this->InteractionState = psiPlanBoxRepresentation::MoveL2P2;
			}
		}
		else //see if the hex is picked
		{
			path = this->GetAssemblyPath3DPoint(pos, this->HexPicker);

			if (path != nullptr)
			{
				this->LastPicker = this->HexPicker;
				this->ValidPick = 1;
				this->CurrentHandle = this->Handle[6];
				this->InteractionState = psiPlanBoxRepresentation::Translating;
			}
			else
			{
				path = this->GetAssemblyPath3DPoint(pos, this->LinePicker);

				if (path != nullptr)
				{
					this->LastPicker = this->LinePicker;
					this->ValidPick = 1;

					this->CurrentHandle = reinterpret_cast<vtkActor *>(path->GetFirstNode()->GetViewProp());
					if (this->CurrentHandle == this->CylinderActor1)
					{
						this->InteractionState = psiPlanBoxRepresentation::MoveL1;
					}
					else if (this->CurrentHandle == this->CylinderActor2)
					{
						this->InteractionState = psiPlanBoxRepresentation::MoveL2;
					}
				}
				else
				{
					//this->InteractionState = psiPlanBoxRepresentation::OutSide;
				}
			}
		}
	}

	return this->InteractionState;
}

//----------------------------------------------------------------------
void psiPlanBoxRepresentation::SetInteractionState(int state)
{
	//// Clamp to allowable values
	//state = (state < psiPlanBoxRepresentation::OutSide ? psiPlanBoxRepresentation::OutSide :
	//	(state > psiPlanBoxRepresentation::Scaling ? psiPlanBoxRepresentation::Scaling : state));

	// Depending on state, highlight appropriate parts of representation
	int handle;
	this->InteractionState = state;
	switch (state)
	{
	case psiPlanBoxRepresentation::MoveF0:
	case psiPlanBoxRepresentation::MoveF1:
	case psiPlanBoxRepresentation::MoveF2:
	case psiPlanBoxRepresentation::MoveF3:
	case psiPlanBoxRepresentation::MoveF4:
	case psiPlanBoxRepresentation::MoveF5:
		handle = this->HighlightHandle(this->CurrentHandle);
		this->HighlightFace(handle);
		break;
	case psiPlanBoxRepresentation::Rotating:
		this->HighlightHandle(nullptr);
		this->HighlightFace(this->HexPicker->GetCellId());
		break;
	case psiPlanBoxRepresentation::Translating:
	case psiPlanBoxRepresentation::Scaling:
		this->HighlightHandle(this->Handle[6]);
		this->HighlightFace(-1);
		break;
	case psiPlanBoxRepresentation::MoveL1P1:
	case psiPlanBoxRepresentation::MoveL1P2:
	case psiPlanBoxRepresentation::MoveL2P1:
	case psiPlanBoxRepresentation::MoveL2P2:
		this->HighlightHandle(this->CurrentHandle);
		this->HighlightFace(-1);
		break;
	case psiPlanBoxRepresentation::MoveL1:
	case psiPlanBoxRepresentation::MoveL2:
		this->HighlightCylinder(this->CurrentHandle);
		this->HighlightFace(-1);
		break;
	default:
		this->HighlightHandle(nullptr);
		this->HighlightFace(-1);
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::BuildRepresentation()
{
	// Rebuild only if necessary
	if (this->GetMTime() > this->BuildTime ||
		(this->Renderer && this->Renderer->GetVTKWindow() &&
		(this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime ||
			this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime)))
	{
		this->SizeHandles();
		this->BuildTime.Modified();
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
	this->HexActor->ReleaseGraphicsResources(w);
	this->HexFace->ReleaseGraphicsResources(w);
	this->CylinderActor1->ReleaseGraphicsResources(w);
	this->CylinderActor2->ReleaseGraphicsResources(w);
	// render the handles
	for (int j = 0; j < 7; j++)
	{
		this->Handle[j]->ReleaseGraphicsResources(w);
	}
	for (int j = 0; j < 4; j++)
	{
		this->LinePointHandle[j]->ReleaseGraphicsResources(w);
	}
}

//----------------------------------------------------------------------------
int psiPlanBoxRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
	int count = 0;
	this->BuildRepresentation();

	this->HexActor->SetPropertyKeys(this->GetPropertyKeys());
	this->HexFace->SetPropertyKeys(this->GetPropertyKeys());

	count += this->HexActor->RenderOpaqueGeometry(v);
	count += this->HexFace->RenderOpaqueGeometry(v);
	count +=this->CylinderActor1->RenderOpaqueGeometry(v);
	count += this->CylinderActor2->RenderOpaqueGeometry(v);
	for (int j = 0; j < 7; j++)
	{
		if (this->Handle[j]->GetVisibility())
		{
			this->Handle[j]->SetPropertyKeys(this->GetPropertyKeys());
			count += this->Handle[j]->RenderOpaqueGeometry(v);
		}
	}

	for (int j = 0; j < 4; j++)
	{
		if (this->LinePointHandle[j]->GetVisibility())
		{
			this->LinePointHandle[j]->SetPropertyKeys(this->GetPropertyKeys());
			count += this->LinePointHandle[j]->RenderOpaqueGeometry(v);
		}
	}

	return count;
}

//----------------------------------------------------------------------------
int psiPlanBoxRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *v)
{
	int count = 0;
	this->BuildRepresentation();

	this->HexActor->SetPropertyKeys(this->GetPropertyKeys());
	this->HexFace->SetPropertyKeys(this->GetPropertyKeys());
	this->CylinderActor1->SetPropertyKeys(this->GetPropertyKeys());
	this->CylinderActor2->SetPropertyKeys(this->GetPropertyKeys());

	count += this->HexActor->RenderTranslucentPolygonalGeometry(v);
	count += this->HexFace->RenderTranslucentPolygonalGeometry(v);
	count += this->CylinderActor1->RenderTranslucentPolygonalGeometry(v);
	count += this->CylinderActor2->RenderTranslucentPolygonalGeometry(v);
	// render the handles
	for (int j = 0; j < 7; j++)
	{
		if (this->Handle[j]->GetVisibility())
		{
			this->Handle[j]->SetPropertyKeys(this->GetPropertyKeys());
			count += this->Handle[j]->RenderTranslucentPolygonalGeometry(v);
		}
	}
	for (int j = 0; j < 4; j++)
	{
		if (this->LinePointHandle[j]->GetVisibility())
		{
			this->LinePointHandle[j]->SetPropertyKeys(this->GetPropertyKeys());
			count += this->LinePointHandle[j]->RenderTranslucentPolygonalGeometry(v);
		}
	}
	return count;
}

//----------------------------------------------------------------------------
vtkTypeBool psiPlanBoxRepresentation::HasTranslucentPolygonalGeometry()
{
	int result = 0;
	this->BuildRepresentation();

	result |= this->HexActor->HasTranslucentPolygonalGeometry();

	// If the face is not selected, we are not really rendering translucent faces,
	// hence don't bother taking it's opacity into consideration.
	// Look at BUG #7301.
	if (this->HexFace->GetProperty() == this->SelectedFaceProperty)
	{
		result |= this->HexFace->HasTranslucentPolygonalGeometry();
	}

	// render the handles
	for (int j = 0; j < 7; j++)
	{
		result |= this->Handle[j]->HasTranslucentPolygonalGeometry();
	}

	return result;
}

void psiPlanBoxRepresentation::SetCylinderDiameter(double diameter)
{
	this->radius = diameter / 2.0;
	tubeFiler1->SetRadius(this->radius);
	tubeFiler1->Update();
	tubeFiler2->SetRadius(this->radius);
	tubeFiler2->Update();
}

#define VTK_AVERAGE(a,b,c) \
  c[0] = (a[0] + b[0])/2.0; \
  c[1] = (a[1] + b[1])/2.0; \
  c[2] = (a[2] + b[2])/2.0;

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::PositionHandles()
{
	double *pts = static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
	double *p0 = pts;
	double *p1 = pts + 3 * 1;
	double *p2 = pts + 3 * 2;
	double *p3 = pts + 3 * 3;
	//double *p4 = pts + 3*4;
	double *p5 = pts + 3 * 5;
	double *p6 = pts + 3 * 6;
	double *p7 = pts + 3 * 7;
	double x[3];

	VTK_AVERAGE(p0, p7, x);
	this->Points->SetPoint(8, x);
	VTK_AVERAGE(p1, p6, x);
	this->Points->SetPoint(9, x);
	VTK_AVERAGE(p0, p5, x);
	this->Points->SetPoint(10, x);
	VTK_AVERAGE(p2, p7, x);
	this->Points->SetPoint(11, x);
	VTK_AVERAGE(p1, p3, x);
	this->Points->SetPoint(12, x);
	VTK_AVERAGE(p5, p7, x);
	this->Points->SetPoint(13, x);
	VTK_AVERAGE(p0, p6, x);
	this->Points->SetPoint(14, x);

	for (int i = 0; i < 7; ++i)
	{
		this->HandleGeometry[i]->SetCenter(this->Points->GetPoint(8 + i));
	}

	for (int i = 0; i < 4; ++i)
	{
		this->LinePointHandleGeometry[i]->SetCenter(this->LinePoints->GetPoint(i));
	}

	this->LinePoints->GetData()->Modified();
	this->LinePointPolyData->Modified();

	this->Line1Source->SetPoint1(this->LinePoints->GetPoint(0));
	this->Line1Source->SetPoint2(this->LinePoints->GetPoint(1));
	this->Line1Source->Update();

	this->Line2Source->SetPoint1(this->LinePoints->GetPoint(2));
	this->Line2Source->SetPoint2(this->LinePoints->GetPoint(3));
	this->Line2Source->Update();

	for (int i = 0; i < 6; ++i)
	{
		this->Planes[i]->SetOrigin(this->Points->GetPoint(8 + i));
		int mix = 2 * (i % 2);
		vtkVector3d pp1(this->Points->GetPoint(8 + i));
		vtkVector3d pp2(this->Points->GetPoint(9 + i - mix));
		pp2 = pp2 - pp1;
		pp2.Normalize();
		this->Planes[i]->SetNormal(pp2.GetData());
	}

	this->Points->GetData()->Modified();
	this->HexFacePolyData->Modified();
	this->HexPolyData->Modified();
}
#undef VTK_AVERAGE

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::HandlesOn()
{
	for (int i = 0; i < 7; i++)
	{
		this->Handle[i]->VisibilityOn();
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::HandlesOff()
{
	for (int i = 0; i < 7; i++)
	{
		this->Handle[i]->VisibilityOff();
	}
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::SizeHandles()
{
	double *center = static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(3 * 14);
	double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, center);
	for (int i = 0; i < 7; i++)
	{
		this->HandleGeometry[i]->SetRadius(radius);
	}

	for (int i = 0; i < 4; i++)
	{
		this->LinePointHandleGeometry[i]->SetRadius(radius);
	}
}

void psiPlanBoxRepresentation::HighlightCylinder(vtkProp *prop)
{
	// first unhighlight anything picked
	if (this->CurrentHandle)
	{
		this->CurrentHandle->SetProperty(this->CylinderProperty);
	}

	this->CurrentHandle = static_cast<vtkActor *>(prop);

	if (this->CurrentHandle)
	{
		this->CurrentHandle->SetProperty(this->SelectedCylinderProperty);
	}
}

//----------------------------------------------------------------------------
int psiPlanBoxRepresentation::HighlightHandle(vtkProp *prop)
{
	// first unhighlight anything picked

	if (this->CurrentHandle)
	{
		this->CurrentHandle->SetProperty(this->HandleProperty);
	}

	this->CurrentHandle = static_cast<vtkActor *>(prop);

	if (this->CurrentHandle)
	{
		this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
		for (int i = 0; i < 6; i++) //find attached face
		{
			if (this->CurrentHandle == this->Handle[i])
			{
				return i;
			}
		}
	}

	if (this->CurrentHandle == this->Handle[6])
	{
		return 6;
	}

	return -1;
}

//----------------------------------------------------------------------------
void psiPlanBoxRepresentation::HighlightFace(int cellId)
{
	if (cellId >= 0)
	{
		vtkIdType npts;
		const vtkIdType *pts;
		vtkCellArray *cells = this->HexFacePolyData->GetPolys();
		this->HexPolyData->GetCellPoints(cellId, npts, pts);
		this->HexFacePolyData->Modified();
		cells->ReplaceCell(0, npts, pts);
		cells->Modified();
		this->CurrentHexFace = cellId;
		this->HexFace->SetProperty(this->SelectedFaceProperty);
		if (!this->CurrentHandle)
		{
			this->CurrentHandle = this->HexFace;
		}
	}
	else
	{
		this->HexFace->SetProperty(this->FaceProperty);
		this->CurrentHexFace = -1;
	}
}

//------------------------------------------------------------------------------
void psiPlanBoxRepresentation::RegisterPickers()
{
	vtkPickingManager* pm = this->GetPickingManager();
	if (!pm)
	{
		return;
	}
	pm->AddPicker(this->HandlePicker, this);
	pm->AddPicker(this->HexPicker, this);
}

void psiPlanBoxRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
	os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");
}