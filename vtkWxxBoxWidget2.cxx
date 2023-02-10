/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWxxBoxWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWxxBoxWidget2.h"
#include "vtkWxxBoxRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkWxxBoxWidget2);

//------------------------------------------------------------------------------
vtkWxxBoxWidget2::vtkWxxBoxWidget2()
{
  this->WidgetState = vtkWxxBoxWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = true;
  this->ScalingEnabled = true;
  this->RotationEnabled = true;
  this->MoveFacesEnabled = true;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, 0,
    0, nullptr, vtkWidgetEvent::Select, this, vtkWxxBoxWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::NoModifier,
    0, 0, nullptr, vtkWidgetEvent::EndSelect, this, vtkWxxBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkWxxBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkWxxBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
    vtkEvent::ControlModifier, 0, 0, nullptr, vtkWidgetEvent::Translate, this,
    vtkWxxBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkEvent::ControlModifier, 0, 0, nullptr, vtkWidgetEvent::EndTranslate, this,
    vtkWxxBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::ShiftModifier,
    0, 0, nullptr, vtkWidgetEvent::Translate, this, vtkWxxBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkEvent::ShiftModifier, 0, 0, nullptr, vtkWidgetEvent::EndTranslate, this,
    vtkWxxBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkWxxBoxWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkWxxBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkWxxBoxWidget2::MoveAction);

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkWxxBoxWidget2::ProcessKeyEvents);
}

//------------------------------------------------------------------------------
vtkWxxBoxWidget2::~vtkWxxBoxWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//------------------------------------------------------------------------------
void vtkWxxBoxWidget2::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // We defer enabling the handles until the selection process begins
  if (enabling && !enabled)
  {
    if (this->Parent)
    {
      this->Parent->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Parent->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
    else
    {
      this->Interactor->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Interactor->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
  }
  else if (!enabling && enabled)
  {
    if (this->Parent)
    {
      this->Parent->RemoveObserver(this->KeyEventCallbackCommand);
    }
    else
    {
      this->Interactor->RemoveObserver(this->KeyEventCallbackCommand);
    }
  }
}

//------------------------------------------------------------------------------
void vtkWxxBoxWidget2::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkWxxBoxWidget2* self = reinterpret_cast<vtkWxxBoxWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkWxxBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkWxxBoxRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == vtkWxxBoxRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == vtkWxxBoxRepresentation::MoveF0 ||
        interactionState == vtkWxxBoxRepresentation::MoveF1 ||
        interactionState == vtkWxxBoxRepresentation::MoveF2 ||
        interactionState == vtkWxxBoxRepresentation::MoveF3 ||
        interactionState == vtkWxxBoxRepresentation::MoveF4 ||
        interactionState == vtkWxxBoxRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == vtkWxxBoxRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkWxxBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // The SetInteractionState has the side effect of highlighting the widget
  reinterpret_cast<vtkWxxBoxRepresentation*>(self->WidgetRep)->SetInteractionState(interactionState);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
// 按住shift或ctrl, 鼠标左击实现
void vtkWxxBoxWidget2::TranslateAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkWxxBoxWidget2* self = reinterpret_cast<vtkWxxBoxWidget2*>(w);

  if (self->TranslationEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkWxxBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkWxxBoxRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkWxxBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkWxxBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkWxxBoxRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
// 鼠标右击实现
void vtkWxxBoxWidget2::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkWxxBoxWidget2* self = reinterpret_cast<vtkWxxBoxWidget2*>(w);

  if (self->ScalingEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkWxxBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkWxxBoxRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkWxxBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkWxxBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkWxxBoxRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
// 选中widget实现, 具体参考vtkWxxBoxRepresentation::WidgetInteraction
void vtkWxxBoxWidget2::MoveAction(vtkAbstractWidget* w)
{
  vtkWxxBoxWidget2* self = reinterpret_cast<vtkWxxBoxWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == vtkWxxBoxWidget2::Start)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkWxxBoxWidget2::EndSelectAction(vtkAbstractWidget* w)
{
  vtkWxxBoxWidget2* self = reinterpret_cast<vtkWxxBoxWidget2*>(w);
  if (self->WidgetState == vtkWxxBoxWidget2::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = vtkWxxBoxWidget2::Start;
  reinterpret_cast<vtkWxxBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkWxxBoxRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkWxxBoxWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkWxxBoxRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkWxxBoxWidget2::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkWxxBoxWidget2* self = static_cast<vtkWxxBoxWidget2*>(clientdata);
  vtkRenderWindowInteractor* iren = self->GetInteractor();
  vtkWxxBoxRepresentation* rep = vtkWxxBoxRepresentation::SafeDownCast(self->WidgetRep);
  switch (event)
  {
    case vtkCommand::KeyPressEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
          rep->SetXTranslationAxisOn();
          break;
        case 'y':
        case 'Y':
          rep->SetYTranslationAxisOn();
          break;
        case 'z':
        case 'Z':
          rep->SetZTranslationAxisOn();
          break;
        default:
          break;
      }
      break;
    case vtkCommand::KeyReleaseEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
        case 'y':
        case 'Y':
        case 'z':
        case 'Z':
          rep->SetTranslationAxisOff();
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWxxBoxWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
  os << indent << "Move Faces Enabled: " << (this->MoveFacesEnabled ? "On\n" : "Off\n");
}
