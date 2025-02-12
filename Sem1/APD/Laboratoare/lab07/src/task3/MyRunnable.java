package task3;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

public class MyRunnable implements Runnable {
	ExecutorService executor;
	int[] graph;
	int step;
	AtomicInteger inQueue = new AtomicInteger(0);

	MyRunnable(ExecutorService executor, int[] graph, int step, AtomicInteger inQueue) {
		this.executor = executor;
		this.graph = graph;
		this.step = step;
		this.inQueue = inQueue;
	}

	@Override
	public void run() {
		if (step == Main.N) {
			Main.printQueens(graph);
			inQueue.decrementAndGet();
		} else {
			for (int i = 0; i < Main.N; i++) {
				int[] newGraph = graph.clone();
				newGraph[step] = i;
				if (Main.check(newGraph, step)) {
					inQueue.incrementAndGet();
					executor.execute(new MyRunnable(executor, newGraph, step + 1, inQueue));
				}
			}
			inQueue.decrementAndGet();
		}

		if (inQueue.get() == 0) {
			executor.shutdown();
		}
	}
}
