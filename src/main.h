#pragma once
#ifndef _MAIN_H_
#define _MAIN_H_

//----------------------------------------------------------------------------
//���������� ������ÿ��ʱ�����ڴ棬�ÿ����������ͷţ�������ʱ������
//��Щ��������ֵ���豣��;
/*
module Flow_Var
real * 8, save, pointer, dimension(:, : )::d, uu, v, T, p, cc  //�ܶȡ�x - �ٶȡ�y - �ٶȡ�ѹ�������٣�
real * 8, save, pointer, dimension(:, : , : )::Fluxi, Fluxj         //i - ��j - �����ͨ��
end module Flow_Var
*/

void output_Res(int nMesh);
void output(int nMesh);


#endif // !_MAIN_H_
