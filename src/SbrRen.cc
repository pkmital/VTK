/*=========================================================================

  Program:   Visualization Library
  Module:    SbrRen.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <iostream.h>
#include "SbrProp.hh"
#include "SbrCam.hh"
#include "SbrLgt.hh"
#include "SbrRenW.hh"
#include "SbrRen.hh"
#include "SbrPoly.hh"
#include "SbrTri.hh"
#include "SbrLine.hh"
#include "SbrPnt.hh"

#define MAX_LIGHTS 16

static float z_ref_matrix[4][4] = {
  1.0,  0.0,  0.0,  0.0,
  0.0,  1.0,  0.0,  0.0,
  0.0,  0.0, -1.0,  0.0,
  0.0,  0.0,  0.0,  1.0
  };

/* stereo definitions : also in starbase_camera.cls */
static char *lights[MAX_LIGHTS] =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

vlSbrRenderer::vlSbrRenderer()
{
}

// Description:
// Ask actors to build and draw themselves.
int vlSbrRenderer::UpdateActors()
{
  vlActor *anActor;
  float visibility;
  vlMatrix4x4 matrix;
  int count = 0;
 
  // loop through actors 
  for ( this->Actors.InitTraversal(); anActor = this->Actors.GetNextItem(); )
    {
    // if it's invisible, we can skip the rest 
    visibility = anActor->GetVisibility();

    if (visibility == 1.0)
      {
      count++;
      // build transformation 
      anActor->GetMatrix(matrix);
      matrix.Transpose();
 
      concat_transformation3d(this->Fd, z_ref_matrix, PRE, PUSH);

      // insert model transformation 
      concat_transformation3d(this->Fd,(float (*)[4])(matrix[0]), PRE, PUSH);

      anActor->Render((vlRenderer *)this);
 
      pop_matrix(this->Fd);
      pop_matrix(this->Fd);
      }
    }
  return count;
}

// Description:
// Ask active camera to load its view matrix.
int vlSbrRenderer::UpdateCameras ()
{
  // update the viewing transformation 
  if (!this->ActiveCamera) return 0;
  
  this->ActiveCamera->Render((vlRenderer *)this);
  return 1;
}

// Description:
// Internal method temporarily removes lights before reloading them
// into graphics pipeline.
void vlSbrRenderer::ClearLights (void)
{
  light_ambient(this->Fd, this->Ambient[0],
		this->Ambient[1], this->Ambient[2]);
  this->LightSwitch = 0x0001;
  
  vlDebugMacro(<< "SB_light_ambient: " << this->Ambient[0] << " " <<
  this->Ambient[1] << " " << this->Ambient[2] << "\n");
  
  light_switch(this->Fd, this->LightSwitch);
 
  vlDebugMacro( << " SB_light_switch: " << this->LightSwitch << "\n");

  this->NumberOfLightsBound = 1;
}

// Description:
// Ask lights to load themselves into graphics pipeline.
int vlSbrRenderer::UpdateLights ()
{
  vlLight *light;
  short cur_light;
  float status;
  int count = 0;

  cur_light= this->NumberOfLightsBound;

  for ( this->Lights.InitTraversal(); light = this->Lights.GetNextItem(); )
    {

    status = light->GetSwitch();

    // if the light is on then define it and bind it. 
    // also make sure we still have room.             
    if ((status > 0.0)&& (cur_light < MAX_LIGHTS))
      {
      light->Render((vlRenderer *)this,cur_light);
      // increment the current light by one 
      cur_light++;
      count++;
      // and do the same for the mirror source if backlit is on
      // and we aren't out of lights
      if ((this->BackLight > 0.0) && 
	  (cur_light < MAX_LIGHTS))
	{
	// if backlighting is on then increment the current light again 
	cur_light++;
	}
      }
    }
  
  this->NumberOfLightsBound = cur_light;
  
  return count;
}
 
// Description:
// Concrete starbase render method.
void vlSbrRenderer::Render(void)
{
  vlSbrRenderWindow *temp;

  // update our Fd first
  temp = (vlSbrRenderWindow *)this->GetRenderWindow();
  this->Fd = temp->GetFd();

  // standard render method 
  this->ClearLights();
  this->DoCameras();
  this->DoLights();
  this->DoActors();
}

// Description:
// Create particular type of starbase geometry primitive.
vlGeometryPrimitive *vlSbrRenderer::GetPrimitive(char *type)
{
  vlGeometryPrimitive *prim;

  if (!strcmp(type,"polygons"))
      {
      prim = new vlSbrPolygons;
      return (vlGeometryPrimitive *)prim;
      }

  if (!strcmp(type,"triangle_strips"))
      {
      prim = new vlSbrTriangleMesh;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"lines"))
      {
      prim = new vlSbrLines;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"points"))
      {
      prim = new vlSbrPoints;
      return (vlGeometryPrimitive *)prim;
      }

  return((vlGeometryPrimitive *)NULL);
}


void vlSbrRenderer::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSbrRenderer::GetClassName()))
    {
    this->vlRenderer::PrintSelf(os,indent);

    os << indent << "Number Of Lights Bound: " << 
      this->NumberOfLightsBound << "\n";
    }
}

