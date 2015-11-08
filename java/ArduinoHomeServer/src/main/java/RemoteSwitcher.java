import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.InetSocketAddress;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Locale;



public class RemoteSwitcher
{
    public static void main(String[] args) throws Exception
    {
//        args=new String[] {"31","D","off"};
        byte systemCode;
        char channel;
        byte on;
        int repeatCount=0;
        try
        {
            if (args.length<3||args.length>4) throw new IllegalArgumentException("Invalid number of arguments");
            systemCode=Byte.parseByte(args[0]);
            if (systemCode<0 || systemCode>31) throw new IllegalArgumentException("System code must be between 0 and 31");
            
            if ("a".equalsIgnoreCase(args[1])) channel='A';
            else if ("b".equalsIgnoreCase(args[1])) channel='B';
            else if ("c".equalsIgnoreCase(args[1])) channel='C';
            else if ("d".equalsIgnoreCase(args[1])) channel='D';
            else if ("e".equalsIgnoreCase(args[1])) channel='E';
            else throw new IllegalArgumentException("Channel must be A...E");
            
            if ("on".equalsIgnoreCase(args[2])) on=1;
            else if ("off".equalsIgnoreCase(args[2])) on=0;
            else throw new IllegalArgumentException("Thrird argument must be on or off");
            
            if (args.length>3) repeatCount=Integer.parseInt(args[3]);
            if (repeatCount<0 || repeatCount>50) throw new IllegalArgumentException("Repeat count must be between 0 and 50");

        }
        catch (Exception ex)
        {
            System.err.println("USAGE: RemoteSwitcher SystemCode Channel on|off [RepeatCount]");
            if (ex.getMessage()!=null)
            {
                System.err.println();
                System.err.println("ERROR: "+ex.getMessage());
            }
            return;
        }
        
        int remotePort=5000;
        String remoteHost="192.168.1.199";
        
        DatagramSocket socket=new DatagramSocket();
        socket.setBroadcast(false);
        
        byte[] payload=new byte[13];
        System.arraycopy("SwitchElro".getBytes(),0,payload,0,10);
        payload[10]=systemCode;
        payload[11]=(byte) channel;
        payload[12]=on;
        
        DatagramPacket packet = new DatagramPacket(payload,payload.length);
        packet.setAddress(Inet4Address.getByName(remoteHost));
        packet.setPort(remotePort);
        
        for (int i=0;i<=repeatCount;i++)
        {
            if (i>0)
            {
                System.err.println("Waiting and repeating...");
                try
                {
                    Thread.sleep(100);
                }
                catch (Exception ex)
                {
                    // ignored;
                }
            }
            
            System.err.println("Sending: SwitchElro "+systemCode+" "+channel+" "+(on==1?"on":"off"));
            socket.send(packet);
            System.err.println("ok");
        }
        
    }

}
