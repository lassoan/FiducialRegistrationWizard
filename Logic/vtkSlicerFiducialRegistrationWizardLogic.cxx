/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// FiducialRegistrationWizard includes
#include "vtkSlicerFiducialRegistrationWizardLogic.h"

// MRML includes
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkLandmarkTransform.h>
#include <vtkPoints.h>

// STD includes
#include <cassert>
#include <sstream>


// Helper methods -------------------------------------------------------------------

vtkPoints* MarkupsFiducialNodeToVTKPoints( vtkMRMLMarkupsFiducialNode* markupsFiducialNode )
{
  vtkPoints* points = vtkPoints::New();
  for ( int i = 0; i < markupsFiducialNode->GetNumberOfFiducials(); i++ )
  {
    double currentFiducial[ 3 ] = { 0, 0, 0 };
    markupsFiducialNode->GetNthFiducialPosition( i, currentFiducial );
    points->InsertNextPoint( currentFiducial );
  }

  return points;
}


// Slicer methods -------------------------------------------------------------------

vtkStandardNewMacro(vtkSlicerFiducialRegistrationWizardLogic);



vtkSlicerFiducialRegistrationWizardLogic::vtkSlicerFiducialRegistrationWizardLogic()
{
}



vtkSlicerFiducialRegistrationWizardLogic::~vtkSlicerFiducialRegistrationWizardLogic()
{
}



void vtkSlicerFiducialRegistrationWizardLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


void vtkSlicerFiducialRegistrationWizardLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}



void vtkSlicerFiducialRegistrationWizardLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}



void vtkSlicerFiducialRegistrationWizardLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}



void vtkSlicerFiducialRegistrationWizardLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}



void vtkSlicerFiducialRegistrationWizardLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}


// Module-specific methods ----------------------------------------------------------

void vtkSlicerFiducialRegistrationWizardLogic
::AddFiducial( vtkMRMLLinearTransformNode* probeTransformNode )
{
  if ( probeTransformNode == NULL )
  {
    return;
  }

  vtkMRMLMarkupsFiducialNode* activeMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetMRMLScene()->GetNodeByID( this->MarkupsLogic->GetActiveListID() ) );

  if ( activeMarkupsFiducialNode == NULL )
  {
    return;
  }
  
  vtkMatrix4x4* transformToWorld = vtkMatrix4x4::New();
  probeTransformNode->GetMatrixTransformToWorld( transformToWorld );
  
  double coord[ 3 ] = { transformToWorld->GetElement( 0, 3 ), transformToWorld->GetElement( 1, 3 ), transformToWorld->GetElement( 2, 3 ) };

  activeMarkupsFiducialNode->AddFiducialFromArray( coord );

  transformToWorld->Delete();
}


std::string vtkSlicerFiducialRegistrationWizardLogic
::CalculateTransform( vtkMRMLMarkupsFiducialNode* fromMarkupsFiducialNode, vtkMRMLMarkupsFiducialNode* toMarkupsFiducialNode, vtkMRMLLinearTransformNode* outputTransform, std::string transformType )
{

  if ( fromMarkupsFiducialNode == NULL || toMarkupsFiducialNode == NULL )
  {
    return "One or more fiducial lists not defined.";
  }

  if ( outputTransform == NULL )
  {
    return "Output transform is not defined.";
  }

  if ( fromMarkupsFiducialNode->GetNumberOfFiducials() < 3 || toMarkupsFiducialNode->GetNumberOfFiducials() < 3 )
  {
    return "One or more fiducial lists has too few fiducials.";
  }

  if ( fromMarkupsFiducialNode->GetNumberOfFiducials() != toMarkupsFiducialNode->GetNumberOfFiducials() )
  {
    return "Fiducial lists have unequal number of fiducials.";
  }

  // Convert the markupsfiducial nodes into vector of itk points
  vtkPoints* fromPoints = MarkupsFiducialNodeToVTKPoints( fromMarkupsFiducialNode );
  vtkPoints* toPoints = MarkupsFiducialNodeToVTKPoints( toMarkupsFiducialNode );

  // Setup the registration
  vtkLandmarkTransform* transform = vtkLandmarkTransform::New();

  transform->SetSourceLandmarks( fromPoints );
  transform->SetTargetLandmarks( toPoints );

  if ( transformType.compare( "Similarity" ) == 0 )
  {
    transform->SetModeToSimilarity();
  }
  else
  {
    transform->SetModeToRigidBody();
  }

  transform->Update();

  // Copy the resulting transform into the outputTransform
  vtkMatrix4x4* calculatedTransform = vtkMatrix4x4::New();
  transform->GetMatrix( calculatedTransform );
  outputTransform->SetAndObserveMatrixTransformToParent( calculatedTransform );

  // Delete stuff // TODO: Use smart pointers
  fromPoints->Delete();
  toPoints->Delete();
  transform->Delete();
  calculatedTransform->Delete();

  return "Success.";
}