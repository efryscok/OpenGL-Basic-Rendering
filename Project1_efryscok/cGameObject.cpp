#include "cGameObject.h"

int cGameObject::m_nextID = 1;

cGameObject::cGameObject() {
	// Flag variables
	this->bIsWireframe = false;
	this->bIsUpdatedByPhysics = false;
	
	// Physics variables
	this->Vx = this->Vy = this->Vz = 0.0f;
	this->Ax = this->Ay = this->Az = 0.0f;
	this->pre_Rot_X = this->pre_Rot_Y = this->pre_Rot_Z = 0.0f;
	this->post_Rot_X = this->post_Rot_Y = this->post_Rot_Z = 0.0f;
	
	this->x = this->y = this->z = 0.0f;
	this->scale = 1.0f;
	this->radius = 0.0f;
	this->lastX = this->lastY = this->lastZ = 0.0f;
	this->solid_B = this->solid_G = this->solid_R = 0.0f;
	
	// Mesh info
	this->meshID = 0;
	this->meshName = "N/A";
	
	// Private members
	this->m_uniqueID = cGameObject::m_nextID;
	cGameObject::m_nextID++;

	return;
}

int cGameObject::getID(void) {
	return this->m_uniqueID;
}
