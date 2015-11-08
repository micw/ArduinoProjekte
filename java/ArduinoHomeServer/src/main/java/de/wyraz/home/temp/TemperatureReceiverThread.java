package de.wyraz.home.temp;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.util.Date;



public class TemperatureReceiverThread extends Thread
{
    protected DatagramSocket socket;
    protected boolean running;
    protected final SensorValueListener listener;
    
    public TemperatureReceiverThread(SensorValueListener listener)
    {
        this.listener=listener;
    }
     
    @Override
    public synchronized void start()
    {
        running=true;
        super.start();
    }
    
    public void shutdown()
    {
        running=false;
        socket.close();
        super.stop();
    }
    
    protected int lastMessageNum=-1;
    
    @Override
    public void run()
    {
        try
        {
            socket=new DatagramSocket(null);
            socket.setReuseAddress(true);
            socket.setBroadcast(true);
            socket.bind(new InetSocketAddress(5000));
        }
        catch (Exception ex)
        {
            throw new RuntimeException(ex);
        }
        byte[] rxbuf = new byte[8192];
        DatagramPacket packet = new DatagramPacket(rxbuf, rxbuf.length);

        while (running) try
        {
            socket.receive(packet);
            byte[] bytes=packet.getData();

            int count=packet.getLength();
            
            if (bytes[0]=='a' && count==6) // DS1820 über 433 MHz (oder direkt)
            {
                int pos=1;
                
                int channel = (bytes[pos++] & 0xFF) 
                        | ((bytes[pos++] & 0xFF) << 8);
                int asInt = (bytes[pos++] & 0xFF) 
                        | ((bytes[pos++] & 0xFF) << 8);
                if ((asInt & 0x8000)!=0) asInt=asInt|0xFFFF0000;
                int sequenceNumber = (bytes[pos++] & 0xFF);
                if (lastMessageNum==sequenceNumber) continue;
                lastMessageNum=sequenceNumber;
                float f=asInt/100f;
                listener.setSensorValue(SensorType.TEMPERATURE,"ds18s20-"+channel, f, sequenceNumber);
                continue;
            }
            
            
            if (bytes[0]=='b' && count==6) // Ein-/Ausschaltsignale Heizung
            {
                int pos=1;
                int value = (bytes[pos++] & 0xFF); 
                int systemCode = (bytes[pos++] & 0xFF); 
                char channelCode = (char) (bytes[pos++] & 0xFF); 
                int minTemp = (bytes[pos++] & 0xFF); 
                int sequenceNumber = (bytes[pos++] & 0xFF);
                if (lastMessageNum==sequenceNumber) continue;
                lastMessageNum=sequenceNumber;
                listener.setSensorValue(SensorType.REMOTE_SWITCH_OUT,"switch-"+systemCode+channelCode, value, sequenceNumber);
                listener.setSensorValue(SensorType.REMOTE_SWITCH_MIN_TEMP,"swtemp-"+systemCode+channelCode, minTemp, sequenceNumber);
                continue;
            }
            
            if (bytes[0]=='c' && count==5) // Empfangene Signale Steckdosen-Fernbedienung
            {
                int pos=1;
                int asInt = (bytes[pos++] & 0xFF) 
                        | ((bytes[pos++] & 0xFF) << 8)
                        | ((bytes[pos++] & 0xFF) << 16)
                        | ((bytes[pos++] & 0xFF) << 24);

                int bits=1;
                boolean valid=true;
                for (int i=0;i<12;i++)
                {
                    int bit=asInt % 3;
                    asInt=asInt/3;
                    if (bit==1)
                    {
                        valid=false;
                        break;
                    }
                    bits=(bits<<1);
                    if (bit==0) bits=bits | 1;
                }
                if (!valid) continue;
                int systemCode=bits&31; // niedrigste 5 bit 
                
                char channel='-';
                for (int i=0;i<5;i++)
                {
                    if ((bits & (1<<(i+5)))!=0)
                    {
                        channel=(char)('A'+i);
                    }
                }

                boolean state;
                int stateVal=(bits>>10) & 3;
                if (stateVal==1) state=true; // 01
                else if (stateVal==2) state=false; // 10
                else continue; // ungültig
                
                System.err.println(new Date()+" SWITCH_IN "+Integer.toString(systemCode)+channel+" - "+((state)?"ON":"OFF"));
                
                continue;
            }
            
            if (bytes[0]=='d' && count==6) // Helligkeit
            {
                int pos=1;
                
                int sensorId = (bytes[pos++] & 0xFF); 
                int brightness = (bytes[pos++] & 0xFF) 
                        | ((bytes[pos++] & 0xFF) << 8);
                int sequenceNumber = (bytes[pos++] & 0xFF);
                if (lastMessageNum==sequenceNumber) continue;
                lastMessageNum=sequenceNumber;
                listener.setSensorValue(SensorType.BRIGHTNESS,"b"+sensorId, brightness, sequenceNumber);
                continue;
            }
            
            System.err.print("Invalid Packet with length of "+count+": ");
            for (int i=0;i<count;i++)
            {
                System.err.print(packet.getData()[i]+" ");
            }
            System.err.println();
            continue;

        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
    }
    
    public enum SensorType
    {
        TEMPERATURE,
        REMOTE_SWITCH_OUT,
        REMOTE_SWITCH_MIN_TEMP,
        BRIGHTNESS
        ;
    }
    
    public static interface SensorValueListener
    {
        public void setSensorValue(SensorType type, String key, float value, int sequenceNumber);
    }

}
