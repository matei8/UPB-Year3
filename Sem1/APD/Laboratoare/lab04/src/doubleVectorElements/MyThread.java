package doubleVectorElements;

public class MyThread extends Thread {
	private final int id;
	private int N = 100000013;
	private int P = 4;
	private int start, end;

	public MyThread(int id) {
		this.id = id;
		this.start = this.id * (N / P);
		this.end = (this.id + 1) * (N / P);
	}

	@Override
	public void run() {
//		for (int i = start; i < end; i++) {
//			v[i] = v[i] * 2;
//		}
	}
}
