public class Task implements Runnable {
	private final int id;
	public Task(int id) {
		this.id = id;
	}

	@Override
	public void run() {
		System.out.println("Hello from thread #" + id);
	}
}
