#include "SPH.h"

Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
	vert1 = v1;
	vert2 = v2;
	vert3 = v3;
	normal = computeNormal(v1, v2, v3);
}

glm::vec3 Triangle::computeNormal(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
	return glm::normalize(glm::cross(v2 - v1, v3 - v1));
}

glm::vec3 Triangle::getV1() {
	return vert1;
}

glm::vec3 Triangle::getV2() {
	return vert2;
}

glm::vec3 Triangle::getV3() {
	return vert3;
}

glm::vec3 Triangle::getNorm() {
	return normal;
}

SPH::SPH(){
	kernel = 0.04f;
	mass = 0.016f;

	Number_Particles = 0;

	World_Size.x = 1.28f;
	World_Size.y = 1.28f;
	World_Size.z = 0.64f;
	
	Grid_Size = World_Size / kernel; 
	Grid_Size.x = (int)glm::floor(Grid_Size.x);
	Grid_Size.y = (int)glm::floor(Grid_Size.y);
	Grid_Size.z = (int)glm::floor(Grid_Size.z);
	Number_Cells = Grid_Size.x * Grid_Size.y * Grid_Size.z;
	std::cout << "grid_size.x = " << Grid_Size.x << std::endl;
	std::cout << "grid_size.y = " << Grid_Size.y << std::endl;
	std::cout << "grid_size.z = " << Grid_Size.z << std::endl;
	std::cout << "number_cells = " << Number_Cells << std::endl;

	Gravity = glm::vec3(0.0f, -18.0f, 0.0f);
	K = 2000;
	Stand_Density = 1200;
	Time_Delta = 0.001f;
	collision = 0.0f;
	viscosity = 8.0f;

	CONSTANT1 = 315.0f/(64.0f * PI * pow(kernel, 9));
	CONSTANT2 = 45.0f/(PI * pow(kernel, 6));

	pci_flag = false;
}

SPH::~SPH(){
	//headArray_cell.clear();
}

void SPH::Init_Particles(int num_particle){
	glm::vec3 pos;
	glm::vec3 vel(0.0f, 0.0f, 0.0f);

	//int count = 0;
	for (float i = World_Size.x * 0.01f; i < World_Size.x * 0.1f; i += kernel*0.9f) {
		for(float j = World_Size.y * 0.5f; j < World_Size.y * 0.8f; j += kernel * 0.3f){
			for (float k = World_Size.z * 0.2f; k < World_Size.z * 0.4f; k += kernel * 0.9f) {

				pos = glm::vec3(i, j, k);
				Init_SingleParticle(pos, vel);
				Number_Particles++;
				//count++;
				//if (count >= num_particle) return;
			}
			
		//if (count == num_particle) break;
		}
	}
	std::cout << "Number of Paticles : " << Number_Particles <<std::endl;
	//std::cout << "particle count = " << count << std::endl;
}

void SPH::Init_SingleParticle(glm::vec3 pos, glm::vec3 vel){
	Particle* p = new Particle;
	p->pos = pos;
	p->vel = vel;
	p->acc = glm::vec3(0.0f, 0.0f, 0.0f);
	p->density = Stand_Density;
	p->nextPart = NULL;
	p->radius = EPSL;

	particleArray.push_back(p);

}

glm::vec3 SPH::Get_CellCoord(glm::vec3 in_particlePos){
	glm::vec3 cellCoord = in_particlePos / kernel;
	cellCoord.x = (int)glm::floor(cellCoord.x);
	//std::cout << "cell_pos.x = " << cellpos.x << std::endl;
	cellCoord.y = (int)glm::floor(cellCoord.y);
	cellCoord.z = (int)glm::floor(cellCoord.z);
	return cellCoord;
}

int SPH::Get_Cell_Index(glm::vec3 in_cellCoord){
	//cell out of range
	if((in_cellCoord.x < 0)||(in_cellCoord.x >= Grid_Size.x)||(in_cellCoord.y < 0)||(in_cellCoord.y >= Grid_Size.y) || (in_cellCoord.z<0)||(in_cellCoord.z >= Grid_Size.z)){
		return -1;
		//std::cout<<"return cell ID -1" <<std::endl;
	}
	//compute cellID given cell coord
	int cellID =in_cellCoord.z*Grid_Size.x*Grid_Size.y +in_cellCoord.y * Grid_Size.x + in_cellCoord.x;

	if(cellID > Number_Cells){
		std::cout<<"Error: cell ID > number of cell";
	}
	return cellID;
}

float SPH::Poly6(float r2){
	return CONSTANT1 * pow(kernel * kernel - r2, 3);
}

float SPH::Spiky(float r){
	return -CONSTANT2 * (kernel - r)* (kernel - r);
}

float SPH::Visco(float r){
	return CONSTANT2 * (kernel - r);
}

//keep tracking of the particles in each kernel
void SPH::Get_1st_Particle_EachCell(){

	FirstParticle_ofEachCell = std::vector<Particle*>(Number_Cells, NULL);
	int cellID;
	std::vector<Particle*>::iterator i;
	for (i = particleArray.begin(); i != particleArray.end(); i++) {
		
		cellID = Get_Cell_Index(Get_CellCoord((*i)->pos));
		//if (cellID > Number_Cells-10) {
			//std::cout << "cell ID = " << cellID << std::endl;
		//}

		//each particle points to the next particle in the same kernel
		if (FirstParticle_ofEachCell[cellID] == NULL) {
			(*i)->nextPart = NULL;
			FirstParticle_ofEachCell[cellID] = *i;
		}
		else {
			(*i)->nextPart = FirstParticle_ofEachCell[cellID];
			FirstParticle_ofEachCell[cellID] = *i;
		}
	}
}


void SPH::Compute_DensityPressure(){
	Particle *nextParticle;
	
	glm::vec3 cellCoord;
	glm::vec3 neighborCellCoord;
	int neighborCellIndex;
	std::vector<Particle*>::iterator iter;
	for (iter = particleArray.begin(); iter != particleArray.end(); iter++) {
		(*iter)->density = 0;
		(*iter)->pressure = 0;
		cellCoord = Get_CellCoord((*iter)->pos);

		//check neighbor kernel
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {

					neighborCellCoord = cellCoord + glm::vec3(i, j, k);
					neighborCellIndex = Get_Cell_Index(neighborCellCoord);
					if (neighborCellIndex == -1) continue;

					nextParticle = FirstParticle_ofEachCell[neighborCellIndex];
					while (nextParticle != NULL) {
						glm::vec3 Distance;
						Distance = (*iter)->pos - nextParticle->pos;
						float dis2 = Distance.x * Distance.x + Distance.y * Distance.y + Distance.z * Distance.z;
						//std::cout << "dis2 = " << dis2 << std::endl;
						if ((dis2 < INF) || (dis2 > kernel* kernel)) {
							nextParticle = nextParticle->nextPart;
							continue;
						}
						(*iter)->density += mass * Poly6(dis2);
						nextParticle = nextParticle->nextPart;
					}
				}
			}
		}
		(*iter)->density += mass * Poly6(0.0f);
		(*iter)->pressure = (pow((*iter)->density / Stand_Density, 7) - 1) * K;

	}
}

void SPH::PCIprocess() {

	std::vector<Particle*>::iterator iter;
	for (iter = particleArray.begin(); iter != particleArray.end(); iter++) {

		while ((*iter)->density> 0.9f* Stand_Density && (*iter)->density < 1.1f*Stand_Density) {
			//for(int i =0; i <1; i++){
			Compute_PredictForce();
			Compute_PredictPos();
			Compute_PCIRpoErr();
		}
	}
}

void SPH::Compute_PredictForce() {

	Particle* nextParticle;
	glm::vec3 cellcoord;
	glm::vec3 neighborCellCoord;
	int neighborCellIndex;

	std::vector<Particle*>::iterator iter;
	for (iter = particleArray.begin(); iter != particleArray.end(); iter++) {
		cellcoord = Get_CellCoord((*iter)->pos);

		//check neighbor kernel
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {

					neighborCellCoord = cellcoord + glm::vec3(i, j, k);
					neighborCellIndex = Get_Cell_Index(neighborCellCoord);
					if (neighborCellIndex == -1) continue;

					nextParticle = FirstParticle_ofEachCell[neighborCellIndex];
					while (nextParticle != NULL) {
						glm::vec3 Distance;
						Distance = (*iter)->pos - nextParticle->pos;
						float dis2 = Distance.x * Distance.x + Distance.y * Distance.y + Distance.z * Distance.z;

						if ((dis2 > INF) && (dis2 < kernel * kernel)) {
							float dis = glm::sqrt(dis2);
							float Volume = mass / nextParticle->density;

							float PredictForce = Volume * ((*iter)->pressure + nextParticle->pressure) / 2.0f * Spiky(dis);
							(*iter)->p_Acc -= Distance * PredictForce / dis;

							glm::vec3 RelativeVel = nextParticle->vel - (*iter)->vel;
							PredictForce = Volume * viscosity * Visco(dis);
							(*iter)->p_Acc += RelativeVel * PredictForce;
						}
						nextParticle = nextParticle->nextPart;
					}
				}
			}
		}

		(*iter)->p_Acc = (*iter)->p_Acc / (*iter)->density + Gravity;
	}

}

void SPH::Compute_PredictPos() {

	std::vector <Particle*>::iterator it_p;
	for (it_p = particleArray.begin(); it_p != particleArray.end(); it_p++) {

		(*it_p)->p_vel += (*it_p)->p_Acc * Time_Delta;
		//clamp velocity
		//if ((*it_p)->vel.y <= -8.0f)(*it_p)->vel.y = -5.0f;
		(*it_p)->p_pos += (*it_p)->p_Acc * Time_Delta;

		//Collision_ParticleTriangle(*it_p);
		//Collision_ParticleBoundary(*it_p);
		//RecycleParticle((*it_p)->pos, (*it_p)->vel);
	}

}

void SPH::Compute_PCIRpoErr() {

	Particle* nextParticle;

	glm::vec3 cellCoord;
	glm::vec3 neighborCellCoord;
	int neighborCellIndex;
	std::vector<Particle*>::iterator iter;
	for (iter = particleArray.begin(); iter != particleArray.end(); iter++) {
		//(*iter)->density = 0;
		//(*iter)->pressure = 0;
		cellCoord = Get_CellCoord((*iter)->p_pos);

		//check neighbor kernel
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {

					neighborCellCoord = cellCoord + glm::vec3(i, j, k);
					neighborCellIndex = Get_Cell_Index(neighborCellCoord);
					if (neighborCellIndex == -1) continue;

					nextParticle = FirstParticle_ofEachCell[neighborCellIndex];
					while (nextParticle != NULL) {
						glm::vec3 Distance;
						Distance = (*iter)->p_pos - nextParticle->p_pos;
						float dis2 = Distance.x * Distance.x + Distance.y * Distance.y + Distance.z * Distance.z;
						//std::cout << "dis2 = " << dis2 << std::endl;
						if ((dis2 < INF) || (dis2 > kernel* kernel)) {
							nextParticle = nextParticle->nextPart;
							continue;
						}
						(*iter)->p_density += mass * Poly6(dis2);
						nextParticle = nextParticle->nextPart;
					}
				}
			}
		}
		(*iter)->p_density += mass * Poly6(0.0f);
		(*iter)->p_pressure = (pow((*iter)->density / Stand_Density, 7) - 1) * K;
	}
}


void SPH::Compute_FinalForce(){
	Particle *nextParticle;
	glm::vec3 cellcoord;
	glm::vec3 neighborCellCoord;
	int neighborCellIndex;

	std::vector<Particle*>::iterator iter;
	for (iter = particleArray.begin(); iter != particleArray.end(); iter++) {
		(*iter)->acc = glm::vec3(0.0f);
		cellcoord = Get_CellCoord((*iter)->pos);

		//check neighbor kernel
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {

					neighborCellCoord = cellcoord + glm::vec3(i, j, k);
					neighborCellIndex = Get_Cell_Index(neighborCellCoord);
					if (neighborCellIndex == -1) continue;

					nextParticle = FirstParticle_ofEachCell[neighborCellIndex];
					while (nextParticle != NULL) {
						glm::vec3 Distance;
						Distance = (*iter)->pos - nextParticle->pos;
						float dis2 = Distance.x * Distance.x + Distance.y * Distance.y + Distance.z * Distance.z;

						if ((dis2 > INF) && (dis2 < kernel * kernel)) {
							float dis = glm::sqrt(dis2);
							float Volume = mass / nextParticle->density;
							float Force = Volume * ((*iter)->pressure + nextParticle->pressure) / 2.0f * Spiky(dis);
							(*iter)->acc -= Distance * Force / dis;

							glm::vec3 RelativeVel = nextParticle->vel - (*iter)->vel;
							Force = Volume * viscosity * Visco(dis);
							(*iter)->acc += RelativeVel * Force;
						}
						nextParticle = nextParticle->nextPart;
					}
				}
			}
		}

		(*iter)->acc = (*iter)->acc / (*iter)->density + Gravity;
	}
}

float SPH::ComputeH(Particle *p, Triangle *t) {
	return glm::dot(p->pos - t->getV1(), t->getNorm());
}

bool SPH::checkCollision(Particle* p, Triangle* t) {

	float h = ComputeH(p, t);

	if (h>0 || h < -EPSL) {
		return false;
	}
	else {
		glm::vec3 temp1 = glm::normalize(glm::cross(p->pos - t->getV1(), t->getV3() - t->getV1()));
		glm::vec3 temp2 = glm::normalize(glm::cross(p->pos - t->getV2(), t->getV1() - t->getV2()));
		glm::vec3 temp3 = glm::normalize(glm::cross(p->pos - t->getV3(), t->getV2() - t->getV3()));
		float result = glm::dot(temp1, temp2) + glm::dot(temp2, temp3) + glm::dot(temp3, temp1);

		if (result < -0.2f) return false;
		else return true;
	}
}


void SPH::Update_ParticlePos(){

	std::vector <Particle*>:: iterator it_p;
	for (it_p = particleArray.begin(); it_p != particleArray.end(); it_p++) {

		(*it_p)->vel += (*it_p)->acc * Time_Delta;
		//clamp velocity
		if ((*it_p)->vel.y <= -8.0f)(*it_p)->vel.y = 0.0f;
		(*it_p)->pos += (*it_p)->vel * Time_Delta;

		Collision_ParticleTriangle(*it_p);
		Collision_ParticleBoundary(*it_p);
		RecycleParticle((*it_p)->pos, (*it_p)->vel);
	}
}

void SPH::RecycleParticle(glm::vec3& p_pos, glm::vec3 & p_vel) {
	if (p_pos.y == 0) {
		p_pos = glm::vec3(glm::linearRand(0.01f, 0.05f), 1.27f, glm::linearRand(0.09f, 0.5f));
		p_vel.x = 0.0f;
	}
}

void SPH::Collision_ParticleTriangle(Particle* p) {

	std::vector<std::vector<Triangle*>>::iterator it_tArray;
	for (it_tArray = in_trangleArray2D.begin(); it_tArray != in_trangleArray2D.end(); it_tArray++) {

		std::vector<Triangle*>::iterator it_t;
		for (it_t = it_tArray->begin(); it_t != it_tArray->end(); it_t++) {

			if (checkCollision(p, *it_t)) {
				if (glm::dot(p->vel, (*it_t)->getNorm()) <= 0) {
					//particle come from the normal side
					p->vel += abs(glm::dot(p->vel, (*it_t)->getNorm())) * (*it_t)->getNorm();
					p->pos += abs(ComputeH(p, *it_t)) * (*it_t)->getNorm();
				}
				else {
					//particle coming from the other side of normal
					p->vel += abs(glm::dot(p->vel, (*it_t)->getNorm())) * (*it_t)->getNorm() * -1.0f;
					p->pos += abs(ComputeH(p, *it_t)) * (*it_t)->getNorm() * -1.0f;
				}
			}
		}
	}
}

void SPH::Collision_ParticleBoundary(Particle* p) {

	if (p->pos.x < 0.0f) {
		//p->vel.x = p->vel.x * Wall_Hit;
		p->vel.x = 0.0f;
		p->pos.x = 0.0f;
	}
	if (p->pos.x >= World_Size.x) {
		//p->vel.x = p->vel.x * Wall_Hit;
		p->vel.x = 0.0f;
		p->pos.x = World_Size.x - 0.0001f;
	}
	if (p->pos.y < 0.0f) {
		//p->vel.y = p->vel.y * Wall_Hit;
		//p->vel.y = 0.0f;
		p->pos.y = 0.0f;
	}
	if (p->pos.y >= World_Size.y) {
		//p->vel.y = p->vel.y * Wall_Hit;
		//p->vel.y = 0.0f;
		p->pos.y = World_Size.y - 0.0001f;
	}
	if (p->pos.z < 0.0f) {
		//p->vel.z = p->vel.z * Wall_Hit;
		p->vel.z = 0.0f;
		p->pos.z = 0.0f;
	}
	if (p->pos.z >= World_Size.z) {
		//p->vel.z = p->vel.z * Wall_Hit;
		p->vel.z = 0.0f;
		p->pos.z = World_Size.z - 0.0001f;
	}

}

void SPH::Update(){

	Get_1st_Particle_EachCell();
	Compute_DensityPressure();
	Compute_FinalForce();
	Update_ParticlePos();

	FirstParticle_ofEachCell.clear();
	//Stand_Density = 1200.0f;
}


void SPH::clearTriangleArray2D() {

	in_trangleArray2D.clear();
	//std::cout << "triangle array 2d clear call" << std::endl;
}

int SPH::Get_Particle_Number(){
	return particleArray.size();
}

glm::vec3 SPH::Get_World_Size(){
	return World_Size;
}

std::vector <Particle*> SPH::Get_ParticleArray() {
	return particleArray;
}

std::vector <std::vector<Triangle*>> SPH::Get_TriangleArray2D() {
	return in_trangleArray2D;
}

void SPH::Clear_ParticleArray() {
	particleArray.clear();
	Number_Particles = 0;
}

void SPH::Set_Gravity(glm::vec3 GravDir) {
	Gravity = GravDir;
}

void SPH::Set_Density(float density) {

	Stand_Density = density;
}

void SPH::PushTriangleToArray2D(std::vector<Triangle* >in_triangleArray) {
	
	in_trangleArray2D.push_back(in_triangleArray);

}

void SPH::PCIToggle() {

	pci_flag = !pci_flag;
}
