#include "global_funcs.h"


// localization
typedef struct _localization_element
{
	wchar_t *english;
	wchar_t *localized;
} localization_element;

int n_localization_element_count;
const int increase_step = 8;	// 2^8 = 256, increase when
localization_element *localization_table = NULL;
HRESULT hr_init_localization = set_localization_language(g_active_language);
void dump();

HRESULT add_localization(const wchar_t *English, const wchar_t *Localized)
{
	if (Localized == NULL)
	{
		Localized = English;
	}

	if ((n_localization_element_count >> increase_step) << increase_step == n_localization_element_count)
	{
		localization_table = (localization_element*)realloc(localization_table, sizeof(localization_element) * (n_localization_element_count + (1<<increase_step)));
	}

	for(int i=0; i<n_localization_element_count; i++)
	{
		if (wcscmp(English, localization_table[i].english) == 0)
		{
			return S_FALSE;
		}
	}

	localization_table[n_localization_element_count].english = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(English)+1));
	wcscpy(localization_table[n_localization_element_count].english, English);
	localization_table[n_localization_element_count].localized = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(Localized)+1));
	wcscpy(localization_table[n_localization_element_count].localized, Localized);

	n_localization_element_count++;

	return S_OK;
}

const wchar_t *C(const wchar_t *English)
{
	for(int i=0; i<n_localization_element_count; i++)
	{
		if (wcscmp(localization_table[i].english, English) == 0)
			return localization_table[i].localized;
	}

	return English;
};
HRESULT set_localization_language(localization_language language)
{
	g_active_language = language;

	for(int i=0; i<n_localization_element_count; i++)
	{
		if (localization_table[i].english)
			free(localization_table[i].english);
		if (localization_table[i].localized)
			free(localization_table[i].localized);
	}
	if (localization_table) free(localization_table);
	localization_table = NULL;
	n_localization_element_count = 0;

	switch (language)
	{
	case ENGLISH:
		{
			/*
			add_localization(L"Open File...");
			add_localization(L"Open BluRay3D");
			add_localization(L"Close");

			add_localization(L"Input Layout");
			add_localization(L"Auto");
			add_localization(L"Side By Side");
			add_localization(L"Top Bottom");
			add_localization(L"Monoscopic");

			add_localization(L"Aspect Ratio");
			add_localization(L"Default");

			add_localization(L"Output Mode");
			add_localization(L"Nvidia 3D Vision");
			add_localization(L"Monoscopic 2D");
			add_localization(L"Interlace Mode");
			add_localization(L"Row Interlace");
			add_localization(L"Line Interlace");
			add_localization(L"Checkboard Interlace");
			add_localization(L"Dual Projector");
			add_localization(L"Horizontal Span Mode");
			add_localization(L"Vertical Span Mode");
			add_localization(L"Independent Mode");
			add_localization(L"3DTV");
			add_localization(L"Half Side By Side");
			add_localization(L"Half Top Bottom");
			add_localization(L"Line Interlace");
			add_localization(L"Naked eye 3D");
			add_localization(L"3D Ready DLP");
			add_localization(L"Anaglyph");
			add_localization(L"Gerneral 120Hz Glasses");

			add_localization(L"Audio");

			add_localization(L"Subtitle");

			add_localization(L"Play/Pause");
			add_localization(L"Play");
			add_localization(L"Pause");
			add_localization(L"Fullscreen");
			add_localization(L"Swap Left/Right");
			add_localization(L"Always Show Right Eye");
			add_localization(L"Exit");

			add_localization(L"No BD Drive Found");
			add_localization(L"Folder...");
			add_localization(L"None");
			add_localization(L"Load Subtitle File...");
			add_localization(L"Load Audio Track...");
			add_localization(L"Font...");
			add_localization(L"Color...");
			add_localization(L" (No Disc)");
			add_localization(L" (No Movie Disc)");
			add_localization(L"Select Folder..");
			add_localization(L"Open File");
			add_localization(L"Language");
			add_localization(L"Feature under development");
			add_localization(L"Open Failed");
			add_localization(L"Display Subtitle");
			add_localization(L"Red - Cyan (Red - Blue, Red- Green)");
			add_localization(L"Custom Color");
			add_localization(L"Anaglyph Left Eye Color...");
			add_localization(L"Anaglyph Right Eye Color...");

			add_localization(L"Enter User ID");
			add_localization(L"This program will exit now, Restart it to use new user id.");
			add_localization(L"Exiting");
			add_localization(L"Activation Failed, Server Response:\n.");
			add_localization(L"Warning");
			add_localization(L"You selected the same monitor!");
			add_localization(L"CUDA Accelaration");
			add_localization(L"CUDA setting will apply on next file play.");
			*/
		}
		break;
	case CHINESE:
		{

			// intel S3D
			add_localization(L"Plase switch to one of device supported 3D reslutions first:\n\n",
							 L"�����л����豸֧�ֵ�3D�ֱ��ʣ�\n\n");
			add_localization(L"No supported device found.",			L"δ�ҵ�֧�ֵ��豸");

			add_localization(L"Monitor",				L"��ʾ��");
			add_localization(L"Open File...",			L"���ļ�...");
			add_localization(L"Open BluRay3D",			L"������3Dԭ��");
			add_localization(L"Close",					L"�ر�");

			add_localization(L"Input Layout",			L"�����ʽ");
			add_localization(L"Auto",					L"�Զ��ж�");
			add_localization(L"Side By Side",			L"���Ҹ�ʽ");
			add_localization(L"Top Bottom",				L"���¸�ʽ");
			add_localization(L"Monoscopic",				L"������ӰƬ");

			add_localization(L"Aspect Ratio",			L"�������");
			add_localization(L"Output Aspect Ratio",	L"��ʾ�����");
			add_localization(L"Default",				L"Ĭ��");
			add_localization(L"Letterbox Cropping",		L"�г��ڱ�");
			add_localization(L"Aspect Ratio Mode",		L"�����ģʽ");
			add_localization(L"Aspect Ratio",			L"�������");
			add_localization(L"Stretch",				L"����");
			add_localization(L"Default(Letterbox)",		L"Ĭ��(�ڱ�)");
			add_localization(L"Horizontal Fill",		L"ˮƽ���");
			add_localization(L"Vertical Fill",			L"��ֱ���");


			add_localization(L"Fullscreen Output",						L"ȫ�����");
			add_localization(L"Fullscreen Output 1",					L"ȫ�����1");
			add_localization(L"Fullscreen Output 2",					L"ȫ�����2");

			add_localization(L"Output 1",					L"���1");
			add_localization(L"Output 2",					L"���2");
			add_localization(L"Output Mode",					L"���ģʽ");
			add_localization(L"Nvidia 3D Vision",				L"Nvidia 3D Vision");
			add_localization(L"AMD HD3D",						L"AMD HD3D");
			add_localization(L"Intel Stereoscopic",			L"Intel ������ʾ");
			add_localization(L"Monoscopic 2D",					L"ƽ��(2D)");
			add_localization(L"Interlace Mode",					L"����ģʽ");
			add_localization(L"Row Interlace",					L"�н���");
			//add_localization(L"Line Interlace",					L"�н���");
			add_localization(L"Checkboard Interlace",			L"���̽���");
			add_localization(L"Dual Projector",					L"˫ͶӰ");
			add_localization(L"Horizontal Span Mode",			L"ˮƽ��Խģʽ");
			add_localization(L"Vertical Span Mode",				L"��ֱ��Խģʽ");
			add_localization(L"Independent Mode",				L"��������ģʽ");
			add_localization(L"3DTV",							L"3D����");
			add_localization(L"Half Side By Side",				L"���Ұ��");
			add_localization(L"Half Top Bottom",				L"���°��");
			add_localization(L"Line Interlace",					L"����ƫ��");
			add_localization(L"Row Interlace",					L"����3D");
			add_localization(L"Anaglyph",						L"��ɫ�۾�");
			add_localization(L"IZ3D Displayer",					L"IZ3D ��ʾ��");
			add_localization(L"Gerneral 120Hz Glasses",			L"��ͨ120Hz�۾�");
			add_localization(L"Checkboard Interlace", L"���̽���");

			add_localization(L"Audio",					L"��Ƶ");
			add_localization(L"Use LAV Audio Decoder",		L"ʹ��������Ƶ������");
			add_localization(L"Audio Decoder setting may not apply until next file play or audio swtiching.",
				L"��Ƶ�����������ã�������Ҫ���²��Ż������л�ʱ��Ч");
			add_localization(L"Use Bitstreaming",			L"ʹ��Դ�����");
			add_localization(L"Bitstreaming setting may not apply until next file play or audio swtiching.",
				L"Դ����������ã�������Ҫ���²��Ż������л�ʱ��Ч");

			add_localization(L"Subtitle",				L"��Ļ");
			add_localization(L"This is first time to load ass/ssa subtilte, font scanning may take one minute or two, the player may looks like hanged, please wait...",
							 L"���ǵ�һ������ass/ssa��Ļ��ɨ�����������Ҫ�����ӣ��ڼ䲥���������޷���Ӧ���������Ժ�...");
			add_localization(L"Please Wait",			L"���Ժ�");

			add_localization(L"Video",					L"��Ƶ");
			add_localization(L"Adjust Color...",					L"����ɫ��...");
			add_localization(L"Forced Deinterlace",		L"ǿ��ȥ������ɨ��");
			add_localization(L"Deinterlacing is not recommended unless you see combing artifacts on moving objects.",
							 L"����ȷ���ڻ����ƶ�ʱ����������״�����������鲻Ҫǿ��ȥ������ɨ�衣");

			add_localization(L"Play/Pause\t(Space)",				L"����/��ͣ\t(Space)");
			add_localization(L"Play\t(Space)",					L"����\t(Space)");
			add_localization(L"Pause\t(Space)",					L"��ͣ\t(Space)");
			add_localization(L"Fullscreen\t(Alt+Enter)",				L"ȫ��\t(Alt+Enter)");
			add_localization(L"Always Show Right Eye",	L"������ʾ����");
			add_localization(L"Swap Left/Right\t(Tab)",		L"�����۶Ե�\t(Tab)");
			add_localization(L"Exit",					L"�˳�");

			add_localization(L"No BD Drive Found",		L"δ�ҵ�����������");
			add_localization(L"Folder...",				L"�ļ���...");
			add_localization(L"None",					L"��");
			add_localization(L"Load Subtitle File...",	L"���ļ�����...");
			add_localization(L"Load Audio Track...",	L"�����ⲿ����...");
			add_localization(L"Font...",				L"����...");
			add_localization(L"Color...",				L"��ɫ...");
			add_localization(L"Latency && Stretch...",	L"�ӳ�������...");
			add_localization(L" (No Disc)",				L" (�޹���)");
			add_localization(L" (No Movie Disc)",		L" (�ǵ�Ӱ����)");
			add_localization(L"Select Folder..",		L"ѡ���ļ���..");
			add_localization(L"Open File",				L"���ļ�");
			add_localization(L"Language",				L"����");
			add_localization(L"Feature under development",L"��δ��ɵĹ���");
			add_localization(L"Open Failed",			L"��ʧ��");
			add_localization(L"Display Subtitle",		L"��ʾ��Ļ");
			add_localization(L"Red - Cyan (Red - Blue, Red- Green)",	L"���� (���� ����)");
			add_localization(L"Custom Color",			L"�Զ�����ɫ");
			add_localization(L"Anaglyph Left Eye Color...",L"���۾���ɫ...");
			add_localization(L"Anaglyph Right Eye Color...",L"���۾���ɫ...");

			add_localization(L"Enter User ID",			L"�����û���");
			add_localization(L"This program will exit now, Restart it to use new user id.",	L"������ɣ����ڳ����Զ��˳������û������´���������Ч");
			add_localization(L"Exiting",				L"�����˳�");
			add_localization(L"Activation Failed, Server Response:\n.", L"����ʧ��, ������������Ϣ��\n");

			add_localization(L"Error",					L"����");
			add_localization(L"Warning",				L"����");
			add_localization(L"You selected the same monitor!",L"ѡ������ͬ����ʾ��!");
			add_localization(L"Use CUDA Accelaration",		L"ʹ��CUDAӲ�����");
			add_localization(L"CUDA setting will apply on next file play.",
														L"CUDA���ý�����һ���ļ�����ʱ��Ч");
			add_localization(L"System initialization failed : monitor detection error, the program will exit now.",
														L"��ʼ��ʧ�ܣ���ʾ�豸���ʧ�ܣ������˳���");
			add_localization(L"System initialization failed : server not found, the program will exit now.",
														L"��ʼ��ʧ�ܣ�����������Ӧ�������˳���");
			add_localization(L"You are using a trial copy of DWindow, each clip will play normally for 10 minutes, after that the picture will become grayscale.\nYou can reopen it to play normally for another 10 minutes.\nRegister to remove this limitation.",
							 L"������ʹ����Ѱ��DWindow, ӰƬ����10���Ӻ���𽥱�Ϊ�ڰף�������������ʽ��û������\n������Ҳ�������´�ӰƬ���ɻָ�ɫ��\nע����ʽ����޴����ƣ�");

			add_localization(L"Downmix To 16bit Stereo", L"���16λ˫������Ƶ");
			add_localization(L"Logged out, the program will exit now, restart the program to login.",
							 L"ע���ɹ������ڳ����˳�����һ������ʱ����Ե�¼�����˺�");
			add_localization(L"Are you sure want to logout?", L"ȷ��Ҫע��ô��");
			add_localization(L"Logout...", L"ע��...");
			add_localization(L"Are you sure?", L"ȷ��?");

			// login window
			add_localization(L"Start Activation", L"��ʼ����");
			add_localization(L"Trial", L"��Ѳ���");
			add_localization(L"Login", L"��¼");
			add_localization(L"OK",    L"ȷ��");
			add_localization(L"Cancel", L"ȡ��");
			add_localization(L"User", L"�û�");
			add_localization(L"Password", L"����");
			
			// latency/stretch window
			add_localization(L"Delay/Stretch", L"�ӳ�/����");
			add_localization(L"Delay", L"�ӳ�");
			add_localization(L"Stretch", L"����");
			add_localization(L"Reset", L"����");
			add_localization(L"Ms", L"����");
			add_localization(L"Warning: negative delay or stretch less than 1.0 can cause internal subtitle loss, external subtitle is not affected.",
				L"ע�⣬��ǰ/���̹��ܶ�������Ļ֧�ֽϲ����ʱ������Ҫ����10�룬������ܳ�����Ļ��ʧ���ӳ�/���칦�ܲ������ƣ�������Ļ�಻�����ơ�");

			// color adjustment
			add_localization(L"Adjust Color", L"ɫ�ʵ���");
			add_localization(L"Preview", L"Ԥ��");
			add_localization(L"SyncAdjust", L"˫��ͬʱ����");
			add_localization(L"Left Eye", L"����");
			add_localization(L"Right Eye", L"����");
			add_localization(L"Luminance", L"����");
			add_localization(L"Saturation", L"���Ͷ�");
			add_localization(L"Hue", L"ɫ��");
			add_localization(L"Contrast", L"�Աȶ�");


			// free version
			add_localization(L"Dual projector and IZ3D mode is only available in registered version.", L"˫ͶӰ��IZ3D���ܽ��ڸ��Ѱ����");
			add_localization(L"External audio track support is only available in registered version.", L"�����ⲿ���칦�ܽ��ڸ��Ѱ����");


			// ass fonts
			add_localization(L"(Fonts Loading)", L"���������������У�");

			// Media Info
			add_localization(L"Media Infomation...", L"�ļ���Ϣ...");
			add_localization(L"MediaInfoLanguageFile", L"language\\MediaInfoCN");
			add_localization(L"Reading Infomation ....", L"���ڶ�ȡ��Ϣ");

			add_localization(L"Display Orientation", L"��ʾ������");
			add_localization(L"Horizontal", L"����");
			add_localization(L"Vertical", L"����");


			// Open Double File
			add_localization(L"Open Left And Right File...", L"�����ҷ����ļ�...");
			add_localization(L"Left File:", L"�����ļ�");
			add_localization(L"Right File:", L"�����ļ�");

		}
		break;
	}

	dump();

	return S_OK;
}

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer);

void dump()
{
	char bom[2] = {0xff, 0xfe};
	wchar_t tmp[10240];

	FILE *f = fopen("Z:\\lang.txt", "wb");
	if (f)
	{
		fwrite(bom, 1, 2, f);
		fwprintf(f, L"����\r\n");

		for(int i=0; i<n_localization_element_count; i++)
		{
			wcscpy(tmp, localization_table[i].english);
			wcs_replace(tmp, L"\\", L"\\\\");
			wcs_replace(tmp, L"\t", L"\\t");
			wcs_replace(tmp, L"\n", L"\\n");
			fwprintf(f, L"%s\r\n", tmp);

			wcscpy(tmp, localization_table[i].localized);
			wcs_replace(tmp, L"\n", L"\\n");		
			fwprintf(f, L"%s\r\n", tmp);
		}

		fclose(f);
	}
}