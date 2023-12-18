package dummy;

public class Dummy {
    public static MyClass myObj = new MyClass("myObj");
    
    public static int myFunction(int mynumber, String name) {
        System.out.println("Welcome, " + name);
        System.out.println("Your number is: " + mynumber);
        return mynumber * mynumber;
    }

    public static void threadRun() {
        while (true) {
            int result = myFunction(10, "Dummy");
            System.out.println("Result: " + result);
            System.out.println("MyObject: ");
            myObj.printName(3);

            try {
                Thread.sleep(1000);
            } catch (Exception e) {
                System.out.println("An unexpected error happened");
            }
        }
    }
    
    public static void main(String[] args) throws InterruptedException {
        System.out.println("Started Dummy Process");

        if (args.length == 0) {
            System.out.println("Missing library path!");
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }

        int threadCount = 2;
        Thread[] threads = new Thread[threadCount];

        for (int i = 0; i < threadCount; ++i) {
            Thread t = new Thread(new Runnable() {
                public void run() {
                    threadRun();
                }
            });

            threads[i] = t;
            t.start();
        }

        for (Thread t : threads) {
            t.join();
        }
    }
}
