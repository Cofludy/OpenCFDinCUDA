#pragma once

//this file define the cosntant parameter 
//which is global to the whole code

#ifndef _CONST_VAR_H_
#define	_GLOBAL_VAR_

extern double PI, P_rato_limit;
extern int Scheme_UD1, Scheme_NND2, Scheme_UD3, Scheme_WENO3, Scheme_MUSCL2, Scheme_MUSCL3, Scheme_OMUSCL2;
extern int Flux_Steger_Warming, Flux_HLL, Flux_HLLC, Flux_Roe, Flux_VanLeer, Flux_Ausm;
extern int Reconst_Original, Reconst_Conservative, Reconst_Characteristic;
extern int BC_Wall, BC_Farfield, BC_Symmetry_or_slidewall, BC_Inlet, BC_Outlet, BC_Move_Wall;
extern int Time_Euler1, Time_RK3, Time_LU_SGS, Time_Dual_LU_SGS;
extern int Turbulence_NONE, Turbulence_BL, Turbulence_SA, Turbulence_SST;
extern int LAP;		// ��Ϳ�֮��Ľ�����(overlap)��� ��LAP=2���֧��4�ף�LAP=3���֧��6�ף�LAP=4���֧��8�׾��ȣ�
extern double PrT;	//����Plandtl��
extern int Method_FVM, Method_FDM;   //�������������ַ�
extern int LFDM;    //��ַ���ı߽����񣨿��ǵ��� - �����Ӵ�������Ĳ��⻬�ԣ����Ե��4������ʹ�ò�ַ���


#endif // !_CONST_VAR_H_

