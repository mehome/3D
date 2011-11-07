#include <Windows.h>
#include <stdio.h>
#include <locale.h>
#include <conio.h>
//#include <vld.h>
#include "srt_parser.h"

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer)
{
	const int tmp_size = 2048;
pass:
	wchar_t tmp[tmp_size];
	if (wcslen(to_replace) > tmp_size-1)
		return false;

	wchar_t *left_part = to_replace;
	wchar_t *right_part = wcsstr(to_replace, searchfor);
	if (!right_part)
		return true;

	right_part[0] = NULL;
	wsprintfW(tmp, L"%s%s%s", left_part, replacer, right_part + wcslen(searchfor));
	wcscpy(to_replace, tmp);
	goto pass;


	return false;	// ...
}

int wmain(int argc, wchar_t * argv[])
{
	setlocale( LC_ALL, "CHS" );

	if (argc<4 || argc>5)
	{
		printf("�÷���\n");
		printf("srt_offset.exe [Դ��Ļ] [ƫ�������ļ�] [�����Ļ] [FPS(��ѡ, 24��60, Ĭ��24)]\n");
		printf("���ӣ�\n");
		printf("srt_offset.exe avt.srt avt.txt avt_offset.srt\n");
		printf("srt_offset.exe game.srt game.txt game_offset.srt 60\n");

		printf("��������˳�.\n");
		getch();
		return 0;
	}

	srt_parser srt;
	srt.init(5000, 800000);
	int fps = 24;
	if (argc == 5) fps = _wtoi(argv[4]);
	wprintf(L"���� %s...", argv[1]);
	int o = srt.load(argv[1]);
	printf("%s\n", o==-1?"ʧ��":"OK");
	wprintf(L"���� %s, fps=%.3f...", argv[2], (float)fps/1.001);
	o = srt.load_offset_metadata(argv[2], fps);
	printf("%s\n", o==-1?"ʧ��":"OK");
	wprintf(L"���浽 %s...", argv[2]);
	o = srt.save(argv[3]);
	printf("%s\n", o==-1?"ʧ��":"OK");

	printf("��ɡ���������˳�.\n");
	getch();

	return 0;
}