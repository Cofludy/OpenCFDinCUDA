#pragma once

//This file define the global parameter include 
//physical parameter and geometry parameters


#ifndef _GLOBAL_VAR_H_
#define _GLOBAL_VAR_H_

//#include<stdio.h>
//#include<stdlib.h>

#include "const_var.h"

//�߽�������Ϣ
struct  BC_MSG_TYPE
{
	int f_no, face, ist, iend, jst, jend, neighb, subface, orient;
};

//------------------------------------�����--------------------------------------
//�ṹ�壺����������Ϣ���������α����������������Ϣ��
struct Block_TYPE
{
	int Block_no, nx, ny, subface;           //��ţ�������nx, ny��������
	int FVM_FDM;		//������������������޲�ַ�
	double ** x, **y;	//(x,y) : coordinate of vortex; 
	double **x1, **y1;	//(x1, y1) : coordinate of cell center
	double *** U, ***Un, ***Un1;     //�غ����(��ʱ�䲽��ǰ1, 2��ʱ�䲽��ֵ), conversation variables
	double *** Res;		//�в�
	double ** dt;	//���ֲ���ʱ�䲽��
	double ***	QF;		//ǿ�Ⱥ��������������д�����ʹ�ã�
	double ***	deltU;   //�غ�����Ĳ�ֵ, dU = U(n + 1) - U(n)  ��������ʹ��
	double *** dU;       //�غ�����Ĳ�ֵdU = U(n + 1) - U(n), LU - SGS������ʹ��
	double **Amu, **Amu_t;	//����ճ��ϵ��������ճ��ϵ��
//������: vol���������; si, sj i, j - ���������߽糤��; (ni1, ni2) i - ���������߽編����; (nj1, nj2) j - ���������߽編����;
	double ** vol, **si, ** sj, **ni1, ** ni2, ** nj1, ** nj2;
	double ** Lci, ** Lcj, ** Lvi, ** Lvj;       //�װ뾶(Lci, Lcj i - , j - ������ճ���װ뾶; Lvi, Lvj i - , j - ����ճ�����װ뾶)
	double ** dw;                //������ľ��� ��k - w SSTģ����ʹ�ã�
	BC_MSG_TYPE * bc_msg;    //�߽�������Ϣ                                        
};


//-------------------------- - ����--------------------------------------------------------
//(�絥������ֻ��1�ף���������񣬿����ж���)
struct Mesh_TYPE
{
	//���ݽṹ�����񡱣� �������α��������������Ϣ
	int Mesh_no, Num_Block, Num_Cell, Kstep;        //������(1��Ϊ��ϸ����2��Ϊ������ 3��Ϊ��������...)�����������������Ŀ, ʱ�䲽
	int Nvar;	//���������̣���Ŀ����ʹ��k - wģ�ͣ�����6������ ��ϡ����ʹ����ģ�ͣ������������4����
	double Res_max[7], Res_rms[7], tt;                 //���в�������в�, �ƽ���ʱ��
	Block_TYPE * Block;     //������顱  �������ڡ����񡱣�
	
	//���Ʋ��������ڿ�����ֵ������ͨ������������ģ�͵�
	//��Щ���Ʋ��������ڡ����񡱣���ͬ�����񡱿��Բ��ò�ͬ�ļ��㷽��������ģ�͵ȡ�	 �����磬�������õ;��ȷ�����������ʹ������ģ��, ...��
	int Iflag_turbulence_model, Iflag_Scheme, IFlag_flux, IFlag_Reconstruction;
};              
//-------------------------------------------------------------------------------------------- -

//global variables                                       ���ӳ�����ɼ���ȫ�ֱ���
//----------------------------------------------------------------------------
extern Mesh_TYPE * Mesh;       //������ ������
extern int Num_Mesh;                            //���������
extern int Nvar;
extern int Kstep_save, If_viscous, Iflag_turbulence_model, Iflag_init;
extern int Iflag_Scheme, Iflag_Flux, Iflag_local_dt, IFlag_Reconstruction, Time_Method, Kstep_show, Num_Threads;
extern double Ma, Re, gamma, Pr, AoA, Cp, Cv, t_end, p_outlet, T_inf, Twall, vt_inf, Kt_inf, Wt_inf;
extern double dt_global, CFL, dtmax, dtmin;                                        //��ʱ�䲽���йص���
extern double Res_Inner_Limit;     //�ڵ����в�����
extern double Pre_Step_Mesh[4];
extern double Nstep_Inner_Limit;                 //������ֵʱ��������Ԥ���������� �ڵ�����������

extern int Iflag1;
//---------------------------------------------- -
//ȫ�ֿ��Ʋ�����������ֵ������ͨ������������ģ�͵� ����Щֻ����ϸ������Ч��
//Nvar���̣�����������Ŀ�� ��ʹ��BLģ��Nvar = 4;  ʹ��SAģ�� Nvar = 5, ��ʹ��K - W SSTģ�� Nvar = 6 (4���������� + k���� + w���̣�
	//���Ʊ���  If_viscous = 0 Euler���̣�1 N_S���̣�
	//Iflag_turbulence_model ����ģ�ͣ�BL, SA, SST��;
//Iflag_Scheme ��ֵ��ʽ��
//Iflag_flux ͨ��������
//Iflag_local_dt �Ƿ���þֲ�ʱ�䲽��;
//Num_Threads OpenMP���õ��߳���
//global parameter(for all Meshes)                     ��������, ��ȫ�塰���񡱶�����
//Ma: Mach��;  Re: Reynolds��; Pr: Prandtl��; Cp, Cv: ��ѹ�����ݱ���;
//t_end: End time ���������ʱ��; P_outlet: ����ѹ������������������������, ����������;  Twall: ���£�������������λK��
//T_inf : �����¶�(������ֵ����λK), ��surthland��ʽ��ʹ��;
//Kt_inf, Wt_inf: �����Ķ��ܡ����ܱȺ�ɢ��(Amut_inf = Kt_inf / Wt_inf), SSTģ�͵ĳ�ֵ����ڱ߽�����ʹ��
//vt_inf : ����������ճ��ϵ���������ճ��ϵ��֮�ȣ��� SAģ��ʹ��



#define debug  true

void outputDebug();

extern bool USEGPU;
extern int * transferInt_dev;
extern double * transferDouble_dev;
extern double * x1_dev;
extern double * y1_dev;
extern double * x_dev;
extern double * y_dev;


#endif // _GLOBAL_VAR_H_

