#include"Global_var.h"

extern Mesh_TYPE * Mesh=nullptr;       //������ ������
extern int Num_Mesh = 1;                            //���������
extern int Nvar=4;
extern int Kstep_save=0, If_viscous = 0, Iflag_turbulence_model = 0, Iflag_init = 0;
extern int Iflag_Scheme = 0, Iflag_Flux = 0, Iflag_local_dt = 0, IFlag_Reconstruction = 0, Time_Method = 0, Kstep_show = 0, Num_Threads = 0;
extern double Ma = 0, Re = 0, gamma = 0, Pr = 0, AoA = 0, Cp = 0, Cv = 0, t_end = 0, p_outlet = 0, T_inf = 0, Twall = 0, vt_inf = 0, Kt_inf = 0, Wt_inf = 0;
extern double dt_global = 0, CFL = 0, dtmax = 0, dtmin = 0;                                        //��ʱ�䲽���йص���
extern double Res_Inner_Limit = 0;     //�ڵ����в�����
extern double Pre_Step_Mesh[4] = {0};
extern double Nstep_Inner_Limit = 0;

extern int Iflag1=0;
