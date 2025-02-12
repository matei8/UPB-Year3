package oneProducerOneConsumer;

public class Buffer {
    private int a = -1;
//	private boolean plin = false;

    void put(int value) {
		synchronized (this) {
			try {
				while (a != -1) {
					wait();
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
			}

			a = value;
			notifyAll();
		}
    }

    int get() {
		synchronized (this) {
			try  {
				while (a == -1) {
					wait();
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
				return -1;
			}

			int value = a;
			a = -1;
			notifyAll();
			return value;
		}
    }
}
