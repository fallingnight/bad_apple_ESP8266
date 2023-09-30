/*
 * 别问我为什么一个java一个python，问就是上学期作业改的（
 * 注：本机ip地址可以进powershell用ipconfig看
 * 
 */


import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class FileServer_badapple {
    static final int FRAME_SIZE =704;
    static final int HEAD_SIZE =9;
    static String BA_FILENAME="G:\\BA\\picfile4"; //请改成你自己存放图片数据的文件夹

    class Worker implements Runnable {

        Socket socket;

        Worker(Socket socket) {
            this.socket = socket;
        }

        //把数据读全，但在这里可能不太需要，因为常常是板子那端读不全
        public static byte[] readFullBytes(int length, InputStream inputStream) throws IOException {
            int rest = length;
            byte[] result = new byte[length];
            while (rest != 0) {
                rest = length - inputStream.read(result, length - rest, rest);
            }
            return result;
        }

        //按照文件名的数字大小顺序读取文件夹中的全部文件
        public static void getAllFiles(File folder,OutputStream ost) throws IOException, InterruptedException {
        List<File> filelist=Arrays.asList(folder.listFiles());
        Collections.sort(filelist,new Comparator<File>() {
            @Override
            public int compare(File o1, File o2){
                String name1=o1.getName();
                String name2=o2.getName();
                int lastdot1=o1.getName().lastIndexOf(".");
                int lastdot2=o2.getName().lastIndexOf(".");
                int oid1=Integer.parseInt(name1.substring(0, lastdot1));
                int oid2=Integer.parseInt(name2.substring(0, lastdot2));
                return oid1-oid2;
            }
            
        });
        for (File item : filelist) {
            if (!item.isDirectory()) {
                String filename = item.getAbsolutePath();
                byte[] data_head= new byte[HEAD_SIZE];
                byte[] data = new byte[FRAME_SIZE];
                File infile = new File(filename);
                InputStream ist = new FileInputStream(infile);
                data_head=readFullBytes(data_head.length,ist);
                data=readFullBytes(data.length,ist);
                ost.write(data);
                ist.close();
                System.out.println(infile.getName());
                //Thread.sleep(10); //可在此处控制每帧传输的延时
                

            }
        }
    }


        @Override
        public void run() {
            try {
                String filePath = BA_FILENAME;
                File folder = new File(filePath);
                System.out.println("connected");
                OutputStream ost = socket.getOutputStream();
                Thread.sleep(1000);
                getAllFiles(folder, ost);
                ost.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    //开server
    public static void main(String[] args) throws IOException {
        FileServer_badapple server = new FileServer_badapple();
        server.launch();
    }

    void launch() throws IOException {
        int portNumber = 12001; //端口号
        ServerSocket serverSocket = null;
        try {
            serverSocket = new ServerSocket(portNumber);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                (new Thread(new Worker(clientSocket))).start();
            }
        } catch (Exception e) {
            System.out.println(e.getMessage());
        } finally {
            serverSocket.close();
        }
    }

}
