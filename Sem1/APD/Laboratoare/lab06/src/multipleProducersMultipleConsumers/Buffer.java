package multipleProducersMultipleConsumers;

import java.util.concurrent.ArrayBlockingQueue;

public class Buffer {
	private final ArrayBlockingQueue<Integer> queue;

	public Buffer() {
		queue = new ArrayBlockingQueue<>(Main.N);
	}

	void put(int value) {
		try {
			queue.put(value);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

	int get() {
		try {
			return queue.take();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return -1;
	}
}