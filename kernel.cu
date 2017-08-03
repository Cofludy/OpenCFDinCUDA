// this is the main function file 
//this peogram is a 2D fluid simulation using finite volume method with multiblocks structure mesh 
// code is accelerated by GPU device 
// allright keep by the writer cofludy 
// connect Cofludy@gmail.com


#include<fstream>
#include <string>
#include <iomanip>
#include<iostream>

#include "const_var.h"
#include "Global_var.h"
#include"sub_init.h"
#include "sub_boundary.h"
#include "sub_Finite_Difference.h"
#include "sub_turbulence_SST.h"
#include "common.h"
#include "sub_NS_singlegid.h"
#include "postAnalyze.h"


cudaError_t CheckCudaDevice();
void read_parameter();
void set_control_para();
void output_Res(int nMesh);
void output(int nMesh);
void myFree();

int FileOpenFlag = 0;

extern bool USEGPU = true;

int main()
{
	printf("----------------- OpenCFD-EC2D in CUDA ver 2.0.0 --------------------------\n");
	printf("               Copyright by Lee Hiloong,  cofludy@gmail.com\n");
	printf("		  Programming by Lee HiLoong  2017-7-12        \n \n");
    // Add vectors in parallel.
    cudaError_t cudaStatus = CheckCudaDevice();

	read_parameter();	//��ȡ����������������Ϣ

	check_mesh_multigrid();      //�������������������������, ���趨�������������
	Init();						// ��ʼ�����������ݽṹ; ���뼸�μ�������Ϣ
	set_control_para();			//�趨���������ϵĿ�����Ϣ����ֵ������ͨ������������ģ�͡�ʱ���ƽ���ʽ��
	Update_coordinate_buffer();		//����������Ϣ�����������������
	Init_FiniteDifference();		//�趨���޲�ַ������򣬼���Jocabian�任ϵ��

	if (Iflag_turbulence_model == Turbulence_SST || Iflag_turbulence_model == Turbulence_SA) {
		comput_dw();        //������������ģ��㵽����ľ��� ������SA��SSTģ��ʱ��Ҫ�˼���)
	}
	Init_flow();

	//Mesh_TYPE & MP = Mesh[1];
	//Block_TYPE & B = MP.Block[1];        //��nMesh ������ĵ�mBlock��
	//printf("%f,  %f,   %f,   %f", B.U[1][1][1], B.U[1][1][2], B.U[1][1][3], B.U[1][1][4]);
	//PAUSE;

	printf("start ... ...\n");

	//ͳ������ʱ��   
	clock_t start, finish;
	start = clock();


	//------------------------------------------------------------------------
	//ʱ���ƽ������õ������񡢶���������������� ����1��Euler, 3��RK��LU - SGS
	for (; Mesh[1].tt < t_end && Mesh[1].Kstep<2000; ) {
                //��������ʱ���ƽ�(1��Euler, 3��RK, LU - SGS)
			NS_Time_advance(1);

		if (Mesh[1].Kstep % Kstep_show == 0) {
			output_Res(1);         //��ӡ�в�(��������)
		}
		if (Mesh[1].Kstep % Kstep_save == 0) {
			output(1);           //�������(��������)
			/*outputPressureOnWall(1);*/
			outPutVelcoity();
		}

	}

	output(1);
	outPutVelcoity();


	finish = clock();
	double duration = (double)(finish - start) / CLOCKS_PER_SEC;
	std::cout << "running time = " << duration << " s  = " << duration / 60.0 << " min\n\n Press Any Button to Exit" << std::endl; 

    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }
	PAUSE;
	myFree();
    return 0;
}

// Helper function for using CUDA to add vectors in parallel.
cudaError_t CheckCudaDevice()
{
    cudaError_t cudaStatus;

    // Choose which GPU to run on, change this on a multi-GPU system.
	HANDLE_ERROR(cudaSetDevice(0));

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
    }
    
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
    }
    return cudaStatus;
}




//��ȡ����������������Ϣ
void read_parameter()
{
	std::ifstream fcin;
	fcin.open("control.in");
	std::string tempStr;
	getline(fcin, tempStr);
	fcin >> Ma >> Re >> gamma >> AoA >> Pr >> t_end >> Kstep_save >> If_viscous >> Iflag_turbulence_model >> Iflag_init;
	getline(fcin, tempStr); getline(fcin, tempStr);
	fcin >> Iflag_local_dt >> dt_global >> CFL >> dtmax >> dtmin >> Time_Method >> p_outlet >> T_inf >> Twall >> vt_inf >> Kt_inf >> Wt_inf;
	getline(fcin, tempStr); getline(fcin, tempStr);
	fcin >> Iflag_Scheme >> Iflag_Flux >> IFlag_Reconstruction >> Kstep_show;
	getline(fcin, tempStr); getline(fcin, tempStr);
	fcin >> Num_Mesh >> Num_Threads >> Nstep_Inner_Limit >> Res_Inner_Limit;
	getline(fcin, tempStr); getline(fcin, tempStr);
	for (int i = 1; i <= Num_Mesh; ++i) {
		fcin >> Pre_Step_Mesh[i];
	}
	fcin.close();
	//printf("try= %d", Kstep_save);

	if ((Time_Method == Time_LU_SGS || Time_Method == Time_Dual_LU_SGS) && 1 != Num_Mesh) {
		printf("In this version (ver 1.5.1 ), LU_SGS method Do Not support Multigrid !!!\n");
		printf("Please modify 'control.in' to choose single-grid or other time method\n");
	}

	if (Iflag_turbulence_model == Turbulence_SST) {
		Nvar = 6;           //6������(4���������� + �Ķ���k + �Ⱥ�ɢ��w)
	}
	else if (Iflag_turbulence_model == Turbulence_SA) {
		Nvar = 5;
	}
	else {
		Nvar = 4;
	}

	AoA = AoA*PI / 180.e0;
	Cv = 1.e0 / (gamma*(gamma - 1.e0)*Ma*Ma);
	Cp = Cv*gamma;
	Twall = Twall / T_inf;
}

//�趨���������ϵĿ�����Ϣ
void set_control_para()
{
	Mesh_TYPE & MP = Mesh[1];	//��ϸ������
								//��ϸ������ϵĿ��Ʋ����������Ʋ�����ͬ
	MP.Iflag_turbulence_model = Iflag_turbulence_model;
	MP.Iflag_Scheme = Iflag_Scheme;
	MP.IFlag_flux = Iflag_Flux;
	MP.IFlag_Reconstruction = IFlag_Reconstruction;
	MP.Nvar = Nvar;		//���������̣���Ŀ����������ʹ������ģ�ͣ���Ŀ��Nvar��ͬ��

						//�趨�������ϵĿ��Ʋ���
	for (int nMesh = 2; nMesh <= Num_Mesh; ++nMesh) {
		Mesh_TYPE & MP = Mesh[nMesh];
		MP.Iflag_turbulence_model = Turbulence_NONE;
		MP.Iflag_Scheme = Scheme_UD1;
		MP.IFlag_flux = Iflag_Flux;
		MP.IFlag_Reconstruction = IFlag_Reconstruction;
		MP.Nvar = 4;	//���������̣���Ŀ����������ʹ������ģ�ͣ�������ĿΪ4��
	}
}


//-------------------------------------- -
//��ӡ�в���в�;������в
void output_Res(int nMesh)
{
	printf("\n\n  Kstep=  %d , t= %f  \n", Mesh[nMesh].Kstep, Mesh[nMesh].tt);
	printf("----------The Max Residuals are--------     ---Mesh---  %d\n", nMesh);
	for (int i = 1; i <= Nvar; ++i) {
		printf("%13.9f   ", Mesh[nMesh].Res_max[i]);
	}
	printf("\n");
	printf("  The R.M.S Residuals are \n");
	for (int i = 1; i <= Nvar; ++i) {
		printf("%13.9f   ", Mesh[nMesh].Res_rms[i]);
	}
	printf("\n");
	std::ofstream fcout;

	if (!FileOpenFlag) {
		FileOpenFlag = 1;
		fcout.open("Residual.dat");
		fcout.close();
	}

	fcout.open("Residual.dat", std::ios::app);
	fcout << std::setprecision(15);
	if (fcout.is_open()) {
		fcout << Mesh[nMesh].Kstep << "  ";
		for (int i = 1; i <= Nvar; ++i) {
			//printf("%13.9f   ", Mesh[nMesh].Res_max[i]);
			fcout << Mesh[nMesh].Res_max[i] << "  ";
		}
		//fcout << std::endl;
		for (int i = 1; i <= Nvar; ++i) {
			//printf("%13.9f   ", Mesh[nMesh].Res_rms[i]);
			fcout << Mesh[nMesh].Res_rms[i] << "  ";
		}
		fcout << std::endl;
		fcout.close();
	}
}


//----------------------------------------------------------------------
//������μ������� ��tecplot��ʽ��, ��ϸ����flow2d.dat; ������ flow2d - 2.dat; ������� flow2d - 3.dat
void output(int nMesh)
{
	std::string filename;
	if (nMesh == 1) {
		filename = "flow2d.dat";
	}
	else {
		filename = "flow2d-";
		std::string tempStr = std::to_string(nMesh);
		filename = filename + tempStr + ".plt";			// flow2d-2.dat ; flow2d-3.dat 
	}

	if (nMesh == 1) {
		std::string filename1 = "flow2d-";
		std::string tempStr1 = std::to_string(Mesh[1].Kstep);
		filename1 = filename1 + tempStr1 + ".plt";			// flow2d-2.dat ; flow2d-3.dat 
		Mesh_TYPE & MP = Mesh[1];
		std::ofstream fcout;
		fcout.open(filename1);
		fcout << std::setprecision(12);
		fcout << " variables=x,y,d,u,v,T,p " << std::endl;
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE &B = Mesh[nMesh].Block[m];
			fcout << "zone  i= " << B.nx + 1 << "  j= " << B.ny + 1 << std::endl;
			for (int j = LAP; j <= B.ny + LAP; ++j) {
				for (int i = LAP; i <= B.nx + LAP; ++i) {
					double d1 = B.U[i][j][1];
					double u1 = B.U[i][j][2] / d1;
					double v1 = B.U[i][j][3] / d1;
					double T1 = (B.U[i][j][4] - 0.50*d1*(u1*u1 + v1*v1)) / (Cv*d1);
					double p1 = d1*T1 / (gamma*Ma*Ma);
					fcout << B.x1[i][j] << "  " << B.y1[i][j] << " " << d1 << " " << u1 << " " << v1 << " " << T1 << " " << p1 << std::endl;
				}
			}
		}
		fcout.close();
	}

	printf("write data file ...\n");
	Mesh_TYPE & MP = Mesh[nMesh];
	std::ofstream fcout;
	fcout.open(filename);
	fcout << std::setprecision(12);
	fcout << " variables=x,y,d,u,v,T,p,Amut " << std::endl;
	for (int m = 1; m <= MP.Num_Block; ++m) {
		Block_TYPE &B = Mesh[nMesh].Block[m];
		fcout << "zone  i= " << B.nx + 1 << "  j= " << B.ny + 1 << std::endl;
		for (int j = LAP; j <= B.ny + LAP; ++j) {
			for (int i = LAP; i <= B.nx + LAP; ++i) {
				double d1 = B.U[i][j][1];
				double u1 = B.U[i][j][2] / d1;
				double v1 = B.U[i][j][3] / d1;
				double T1 = (B.U[i][j][4] - 0.50*d1*(u1*u1 + v1*v1)) / (Cv*d1);
				fcout << B.x1[i][j] << "  " << B.y1[i][j] << " " << d1 << " " << u1 << " " << v1 << " " << T1 << " ";
				fcout << d1*T1 / (gamma*Ma*Ma) << "  " << B.Amu_t[i][j] * Re << std::endl;
			}
		}
	}
	fcout.close();

	if (MP.Nvar == 5) {
		fcout.open("SA2d.dat");
		fcout << std::setprecision(12);
		fcout << " variables=x,y,vt " << std::endl;
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE &B = Mesh[nMesh].Block[m];
			fcout << "zone  i= " << B.nx + 1 << "  j= " << B.ny + 1 << std::endl;
			for (int j = LAP; j <= B.ny + LAP; ++j) {
				for (int i = LAP; i <= B.nx + LAP; ++i) {
					fcout << B.x1[i][j] << " " << B.y1[i][j] << " " << B.U[i][j][5] << std::endl;
				}
			}
		}
		fcout.close();
	}

	if (MP.Nvar == 6) {
		fcout.open("SST2D.dat");
		fcout << std::setprecision(12);
		fcout << " variables=x,y,Kt, Wt " << std::endl;
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE &B = Mesh[nMesh].Block[m];
			fcout << "zone  i= " << B.nx + 1 << "  j= " << B.ny + 1 << std::endl;
			for (int j = LAP; j <= B.ny + LAP; ++j) {
				for (int i = LAP; i <= B.nx + LAP; ++i) {
					fcout << B.x1[i][j] << " " << B.y1[i][j] << " " << B.U[i][j][5] << " " << B.U[i][j][6] << std::endl;
				}
			}
		}
		fcout.close();
	}

}

void outputDebug()
{
#if debug
	Mesh_TYPE & MP = Mesh[1];
	std::ofstream fcout;
	fcout.open("outputDebug.plt");
	fcout << std::setprecision(12);
	fcout << " variables=x,y,d,u,v,T,p " << std::endl;
	for (int m = 1; m <= MP.Num_Block; ++m) {
		Block_TYPE &B = Mesh[1].Block[m];
		fcout << "zone  i= " << B.nx + 1 << "  j= " << B.ny + 1 << std::endl;
		for (int j = 1; j <= B.ny + 2 * LAP - 1; ++j) {
			for (int i = 1; i <= B.nx + 2 * LAP - 1; ++i) {
				double d1 = B.U[i][j][1];
				double u1 = B.U[i][j][2] / d1;
				double v1 = B.U[i][j][3] / d1;
				double T1 = (B.U[i][j][4] - 0.50*d1*(u1*u1 + v1*v1)) / (Cv*d1);
				double p1 = d1*T1 / (gamma*Ma*Ma);
				if (abs(d1) > 100 || abs(T1) > 100 || abs(u1) > 100 || abs(v1) > 100) {
					printf("%d, %d, \n %f, %f, %f, %f,", i, j, d1, u1, v1, T1);
					printf("this error is at %s file at %d  line", __FILE__, __LINE__);
					PAUSE;
				}
				fcout << B.x1[i][j] << "  " << B.y1[i][j] << " " << d1 << " " << u1 << " " << v1 << " " << T1 << " " << p1 << std::endl;
			}
		}
	}
	fcout.close();
#endif
}



void myFree()
{
	for (int iMesh = 1; iMesh <= Num_Mesh; ++iMesh) {
		Mesh_TYPE & MP = Mesh[iMesh];

		for (int iBlock = 1; iBlock <= MP.Num_Block; ++iBlock) {
			Block_TYPE & B = MP.Block[iBlock];
			int nx = B.nx;	int ny = B.ny;
			int mm = nx + 2 * LAP;	int nn = ny + 2 * LAP;
			deleteMatrix(B.x, mm);	deleteMatrix(B.y, mm);

			int mm1 = nx + 2 * LAP - 1;	int nn1 = ny + 2 * LAP - 1;
			deleteMatrix(B.x1, mm1);	deleteMatrix(B.y1, mm1);
			deleteMatrix(B.U, mm1, nn1);	deleteMatrix(B.deltU, mm1, nn1);
			deleteMatrix(B.Amu, mm1);	deleteMatrix(B.Amu_t, mm1);

			deleteMatrix(B.vol, nx);
			deleteMatrix(B.si, nx);		deleteMatrix(B.sj, nx);
			deleteMatrix(B.ni1, nx); deleteMatrix(B.ni2, nx);
			deleteMatrix(B.nj1, nx); deleteMatrix(B.nj2, nx);
			//�װ뾶
			deleteMatrix(B.Lci, nx); deleteMatrix(B.Lcj, nx);
			deleteMatrix(B.Lvi, nx); deleteMatrix(B.Lvj, nx);

			deleteMatrix(B.Un, nx, ny);	//��һʱ�䲽��ֵ
			if (Time_Method == Time_Dual_LU_SGS) {
				deleteMatrix(B.Un1, nx);		//n-1ʱ�䲽��ֵ�� ˫ʱ�䲽LU_SGS�����в���
			}
		}
	}
	free(Mesh);
	Mesh = NULL;
}
