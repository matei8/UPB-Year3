public class Main {
	public static void main(String[] args) {
		int N = Runtime.getRuntime().availableProcessors();
		Thread[] threads = new Thread[N];

		for (int i = 0; i < N; i++) {
			threads[i] = new Thread(new Task(i));
			threads[i].start();
		}

		for (int i = 0; i < N; i++) {
			try {
				threads[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
}
