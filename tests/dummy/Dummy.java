package dummy;

public class Dummy {
    public static void main(String[] args) throws InterruptedException {
        System.out.println("Started Dummy Process");

        if (args.length == 0) {
            System.out.println("Missing library path!");
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }
    }
}
