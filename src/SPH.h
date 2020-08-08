#ifndef __SPHSYSTEM_H__
#define __SPHSYSTEM_H__

#include "ObjLibrary/Vector2.h"
#include "stdafx.h"


#define PI 3.141592f
#define INF 1E-12f
#define EPSL 1E-2f

struct Particle
{
		glm::vec3 pos;		
		glm::vec3 vel;		
		glm::vec3 acc;	

		float density;			
		float pressure;		
		float radius;

		Particle* nextPart;		// link list to the next particle in a kernel

		//PCI
		glm::vec3 p_pos;
		glm::vec3 p_vel;
		glm::vec3 p_Acc;
		//PCI
		float p_density;
		float p_pressure;

};


class Triangle
{
public:
	Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);
	//void drawTriangle();
	glm::vec3 getV1();
	glm::vec3 getV2();
	glm::vec3 getV3();
	glm::vec3 getNorm();

private:
	glm::vec3 vert1;
	glm::vec3 vert2;
	glm::vec3 vert3;
	glm::vec3 normal;
	//right hand rule, counter clock-wise
	glm::vec3 computeNormal(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);
};


class SPH{
	private:
		int Number_Particles;			// paticle number
		
		//cell based optimization
		float kernel;					// h in kernel function
		glm::vec3 Grid_Size;			// = worldSize/kernel
		glm::vec3 World_Size;			
		int Number_Cells;				

		//input
		glm::vec3 Gravity;				//
		float mass;					
		float K;						// 
		float Stand_Density;			// 
		float viscosity;

		float Time_Delta;
		float CONSTANT1;
		float CONSTANT2;
		float collision;

		//Particle *Particles;
		std::vector<std::vector<Triangle*>> in_trangleArray2D;		//
		std::vector <Particle*> particleArray;						//
		std::vector <Particle*> FirstParticle_ofEachCell;

		//PCI
		bool pci_flag;

	public:
		SPH(); //initialize variables
		~SPH();
		void Init_Particles(int num_particle);									
		void Init_SingleParticle(glm::vec3 particle_pos, glm::vec3 particle_vel);		
		void PushTriangleArray(Triangle* in_triangle);
		void PushTriangleToArray2D(std::vector<Triangle* >in_triangleArray);

		//math functions
		float Poly6(float r2);		// for density
		float Spiky(float r);		// for pressure
		float Visco(float);			// for viscosity

		void Update();
		void Get_1st_Particle_EachCell();
		void Compute_DensityPressure();
		void Compute_FinalForce();
		void Update_ParticlePos();
		void RecycleParticle(glm::vec3& p_pos, glm::vec3& p_vel);
		//void Update_Collission();
		void Collision_ParticleBoundary(Particle* p);
		void Collision_ParticleTriangle(Particle* p);
		float ComputeH(Particle *p, Triangle *t);

		//PCI
		void PCIprocess();
		void Compute_PredictForce();
		void Compute_PredictPos();
		void Compute_PCIRpoErr();

		//utility
		void PCIToggle();

		glm::vec3 Get_CellCoord(glm::vec3 particle_pos);	
		int Get_Cell_Index(glm::vec3 particle_pos);			
		void clearTriangleArray2D();
		bool checkCollision(Particle* inParticle, Triangle* inTriangle);
		int Get_Particle_Number();
		int Get_Triangle_Number();
		glm::vec3 Get_World_Size();
		std::vector <Particle*> Get_ParticleArray();
		std::vector <std::vector<Triangle*>> Get_TriangleArray2D();
		void Clear_ParticleArray();
		void Set_Gravity(glm::vec3 GravDir);
		void Set_Density(float density);
};


#endif