package multipleProducersMultipleConsumersNBuffer;

import java.util.Queue;

public class Buffer {
    
    Queue<Integer> queue;
    
    public Buffer(int size) {
        queue = new LimitedQueue<>(size);
    }

	public void put(int value) {
		synchronized (this) {
			try {
				while (!queue.isEmpty()) {
					wait();
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
        queue.add(value);
		notifyAll();
	}

	public int get() {
		synchronized (this) {
			try {
				while (queue.isEmpty()) {
					wait();
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
        int a = -1;
        Integer result = queue.poll();
        if (result != null) {
            a = result;
        }
		notifyAll();
        return a;
	}
}
