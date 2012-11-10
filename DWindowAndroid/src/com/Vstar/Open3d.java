/*
 * ��֪���⣺
 * 
 * 1��Sharp�ֻ��ڿ�/��3D������´���Surface������surfaceCreated�¼������������surfaceCreated�е��õĻ��������ѭ��
 *    �����ֻ�����Ҫ��surfaceCreated�¼�֮���ٿ�/��3D�Ż���Ч
 * �����
 *    ��Ҫ��surfaceCreated/surfaceDestroyed�¼��п�/��3D
 *    ��/��3D��һ���̵߳ȴ�surfaceCreated�¼�������handler�������̵߳��� 
 *    ʾ�����룺
 *    private boolean mSurfaceCreated = false;
 *    public void surfaceCreated(SurfaceHolder holder) {mSurfaceCreated = true;}
 *    public void surfaceDestroyed(SurfaceHolder holder) {mSurfaceCreated = false;}
 *    private Handler open3dHandler = new Handler(){public void handleMessage(Message msg) {open3d.Op3d(surfaceView, surfaceHolder, msg.what==1);}}
 *    private void open3d(final boolean open){
 *    	new Thread() {
			public void run() {
				while(!mSurfaceCreated) Thread.sleep(1);
				open3dHandler.sendEmptyMessage(open?1:0);
			}
		}.start();
 *    }
 *    
	};
 *   
 * 2��EVO3Dĳ��4.03�Ĺ̼��ڿ�/��3D�󲻻�������Ч
 * �����
 *    ��/��3D�󵯳�����һ������/toast�Ż���Ч�����Ե�����toast����Ч
 */

package com.Vstar;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class Open3d {
	static{
		System.loadLibrary("3dvOpen3d");
	}
	
	private int Brand3DNo=0;
	private int Brand3DHTC=1;
	private int Brand3DLG=2;
	private int Brand3DSHAP=3;
	private int Brand3d=0;
	
	private native int Get3DBrand(ClassLoader pClassLoader);	
	private native int Open3D(boolean show,SurfaceView mSurfaceView,SurfaceHolder holder,Surface mSurface,boolean issysplay,boolean is3dv, ClassLoader loader);
	public native void delobj();
	private native boolean IsPLR3dv(String path);
	private native Object GetShartObject(Object sv);
	
	public Open3d(){
		try {
			Brand3d = Get3DBrand(getClass().getClassLoader());
		} catch (Exception e) {
		}
	}
	
	public void Op3d(SurfaceView faceview,SurfaceHolder holder,Boolean show3d) {		
		try{
			Open3D(show3d,faceview,holder,holder.getSurface(),false,false, getClass().getClassLoader());
		}catch (Exception e2) {			
		}catch (Error e) {			
		}
}
	
	/**
	 * �ж��Ƿ�������3D�ֻ�
	 * @return
	 */
	public Boolean has3D() {
		if(Brand3d==0)
			return false;
		return true;
	}
}
