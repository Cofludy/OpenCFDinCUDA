#include "sub_NS_multigrid.h"
#include "Global_Var.h"
#include<stdio.h>

//----------------------------------------------------------------------
//��������ϸ����Ĳ�ֵ(Prolong) �� ϸ������������ϲ�ֵ(interpolation)
//----------------------------------------------------------------------
//������m1���غ����(U) ��U�Ĳ��ֵ������m2(��һ��ϸ����)
//flag = 1ʱ����U��ֵ����һ������(׼����ֵʱʹ��)
//flag = 2ʱ����deltU��ֵ����һ������ ��deltU�����ű�ʱ�䲽���ϸ�ʱ�䲽U�Ĳ

void prolong_U(int m1, int m2, int flag)
{
	printf("this prolong file is undefined\n");
	
	//system("pause");

}

//------------------------------------------------------------
//��ǿ�Ⱥ�����ӵ��в��� RF = R + QF
void Add_force_function(int nMesh)
{
	for (int mBlock = 1; Mesh[nMesh].Num_Block; ++mBlock) {
		Block_TYPE & B = Mesh[nMesh].Block[mBlock];
		//$OMP PARALLEL for( int DEFAULT(SHARED) PRIVATE(i, j, m)
		for (int i = 1; i <= B.nx - 1; ++i) {
			for (int j = 1; j <= B.ny - 1; ++j) {
				for (int m = 1; m <= 4; ++m) {
					B.Res[i][j][m] = B.Res[i][j][m] + B.QF[i][j][m];            //���ǿ�Ⱥ�����Ĳв��Դ�����B.Res���� ����ʡ�ڴ棩
				}
			}
		}
		//$OMP END PARALLEL for( int
	}
}
