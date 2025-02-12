package task2;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

import static task2.Main.verifyColors;

public class MyRunnable implements Runnable {
	ExecutorService executor;
	int[] colors;
	int step;
	AtomicInteger inQueue = new AtomicInteger(0);

	public MyRunnable(ExecutorService executor, int[] colors, int step, AtomicInteger inQueue) {
		this.executor = executor;
		this.colors = colors;
		this.step = step;
		this.inQueue = inQueue;
	}

	@Override
	public void run() {
		if (step == Main.N) {
			Main.printColors(colors);
			inQueue.decrementAndGet();
		} else {
			// for the node at position step try all possible colors
			for (int i = 0; i < Main.COLORS; i++) {
				int[] newColors = colors.clone();
				newColors[step] = i;
				if (verifyColors(newColors, step)) {
					inQueue.incrementAndGet();
					executor.execute(new MyRunnable(executor, newColors, step + 1, inQueue));
				}
			}
			inQueue.decrementAndGet();
		}

		if (inQueue.get() == 0) {
			executor.shutdown();
		}
	}
}
