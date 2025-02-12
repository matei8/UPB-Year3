package task1;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

public class MyRunnable implements Runnable {
	ExecutorService tpe;
	List<Integer> partialPath;
	AtomicInteger inQueue = new AtomicInteger(0);
	int destination;

	MyRunnable(ExecutorService tpe, List<Integer> partialPath, AtomicInteger inQueue, int destination) {
		this.tpe = tpe;
		this.partialPath = partialPath;
		this.inQueue = inQueue;
		this.destination = destination;
	}

	@Override
	public void run() {
		int lastNodeInPath = partialPath.get(partialPath.size() - 1);
		for (int[] ints : Main.graph) {
			if (ints[0] == lastNodeInPath) {
				if (partialPath.contains(ints[1]))
					continue;
				List<Integer> newPartialPath = new ArrayList<>(partialPath);
				newPartialPath.add(ints[1]);
				if (ints[1] == Main.destination) {
					System.out.println(newPartialPath);
				} else {
					inQueue.incrementAndGet();
					tpe.submit(new MyRunnable(tpe, newPartialPath, inQueue, destination));
				}
			}
		}
		if (inQueue.decrementAndGet() == 0) {
			tpe.shutdown();
		}
	}
}
