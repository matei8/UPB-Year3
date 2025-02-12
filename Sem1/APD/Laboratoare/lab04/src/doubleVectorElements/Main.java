package doubleVectorElements;

public class Main {

    public static void main(String[] args) {
        int N = 100000013;
        int[] v = new int[N];
        int P = 4; // the program should work for any P <= N

        for (int i = 0; i < N; i++) {
            v[i] = i;
        }

		MyThread[] threads = new MyThread[P];

		for (int i = 0; i < P; i++) {
			threads[i] = new MyThread(i);
			threads[i].start();
		}

		for (int i = 0; i < P; i++) {
			try {
				threads[i].join();
			} catch (InterruptedException e) {
				throw new RuntimeException(e);
			}
		}
        // Parallelize me using P threads


        for (int i = 0; i < N; i++) {
            if (v[i] != i * 2) {
                System.out.println("Wrong answer");
                System.exit(1);
            }
        }
        System.out.println("Correct");
    }
}
