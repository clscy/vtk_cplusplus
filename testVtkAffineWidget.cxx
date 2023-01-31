/*===============================
Author: chenglishuang
Date； 2023.01.16
===============================*/
#include <vtkActor.h>
// #include <vtkAffineRepresentation2D.h>
#include "vtkClsRepresentation2D.h"
// #include "vtkRectangleRepresent2D.h"
// #include <vtkAffineWidget.h>
#include "vtkClsAffineWidget.h"
#include <vtkAppendPolyData.h>
#include <vtkCommand.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>

namespace {
    class vtkAffineCallback : public vtkCommand
    {
      public:
        static vtkAffineCallback* New()
        {
            return new vtkAffineCallback;
        }
        virtual void Execute(vtkObject* caller, unsigned long, void*);
        vtkAffineCallback() : Actor(0), AffineRep(0)
        {
            this->Transform = vtkTransform::New();
        }
        ~vtkAffineCallback()
        {
            this->Transform->Delete();
        }
        vtkActor* Actor;
        // vtkAffineRepresentation2D* AffineRep;
        vtkClsRepresentation2D* AffineRep;
        // vtkRectangleRepresent2D* AffineRep;
        vtkTransform* Transform;
    };
}


int main(int, char*[])
{
    vtkNew<vtkNamedColors> colors;

    vtkNew<vtkSphereSource> sphereSource;
    sphereSource->Update();

    vtkNew<vtkSphereSource> sphereSource2;
    sphereSource2->SetRadius(0.075);
    sphereSource2->SetCenter(0, 0.5, 0);
    sphereSource2->Update();

    vtkNew<vtkAppendPolyData> append;
    append->AddInputConnection(sphereSource->GetOutputPort());
    append->AddInputConnection(sphereSource2->GetOutputPort());
    append->Update();
    
    vtkNew<vtkPlaneSource> planeSource;
    planeSource->SetXResolution(4);
    planeSource->SetYResolution(4);
    planeSource->SetOrigin(-1, -1, 0);
    planeSource->SetPoint1(1, -1, 0);
    planeSource->SetPoint2(-1, 1, 0);
    planeSource->Update();

    vtkNew<vtkPolyDataMapper> planeMapper;
    planeMapper->SetInputConnection(planeSource->GetOutputPort());
    planeMapper->Update();
    vtkNew<vtkActor> planeActor;
    planeActor->SetMapper(planeMapper);
    planeActor->GetProperty()->SetRepresentationToWireframe();
    planeActor->GetProperty()->SetColor(colors->GetColor3d("Red").GetData());

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(append->GetOutputPort());
    mapper->Update();

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);
    renderWindow->SetWindowName("AffineWidget");

    renderer->AddActor(actor);
    renderer->AddActor(planeActor);
    renderer->SetBackground(colors->GetColor3d("LightSkyBlue").GetData()); //
    renderer->SetBackground2(colors->GetColor3d("MidnightBlue").GetData()); //

    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);
    dynamic_cast<vtkInteractorStyleSwitch*>(
        renderWindowInteractor->GetInteractorStyle())->SetCurrentStyleToTrackballCamera();

    // vtkNew<vtkAffineWidget> affineWidget;
    vtkNew<vtkClsAffineWidget> affineWidget;
    affineWidget->SetInteractor(renderWindowInteractor);
    // affineWidget->CreateDefaultRepresentation();
    // dynamic_cast<vtkAffineRepresentation2D*>(affineWidget->GetRepresentation())
    // ->PlaceWidget(actor->GetBounds());
    // dynamic_cast<vtkClsRepresentation2D*>(affineWidget->GetRepresentation())->PlaceWidget(actor->GetBounds());
    vtkNew<vtkClsRepresentation2D> clsRep;
    clsRep->PlaceWidget(actor->GetBounds());
    affineWidget->SetRepresentation(clsRep);

    vtkNew<vtkAffineCallback> affineCallback;
    affineCallback->Actor = actor;
    // affineCallback->AffineRep = dynamic_cast<vtkAffineRepresentation2D*>(
    //     affineWidget->GetRepresentation());
    affineCallback->AffineRep = dynamic_cast<vtkClsRepresentation2D*>(affineWidget->GetRepresentation());
    
    affineWidget->AddObserver(vtkCommand::InteractionEvent, affineCallback);
    affineWidget->AddObserver(vtkCommand::EndInteractionEvent, affineCallback);

    renderWindow->Render();
    renderWindowInteractor->Initialize();
    renderWindow->Render();
    affineWidget->On();

    renderWindowInteractor->Start();
    
    return EXIT_SUCCESS;
}

namespace{
    void vtkAffineCallback::Execute(vtkObject*, unsigned long vtkNotUsed(event),
    void*)
    {
        this->AffineRep->GetTransform(this->Transform);
        this->Actor->SetUserTransform(this->Transform);
    }
}