package net.billchatfield.apps.hostnameservice;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.nio.charset.Charset;
import java.util.logging.Logger;

public class HostnameService {
    
    public static final int PORT = 4140;
    public static final int RESPONSE_PORT = 4141;
    public static final String MULTICAST_ADDR = "233.1.8.69";
    public static final String CHARSET_NAME = "US-ASCII";
    
    private static final Logger logger = Logger.getLogger(HostnameService.class.getName());

    /**
     * Executions start here.
     * 
     * @param args Command line arguments
     */
    public static void main(String[] args) {
        MulticastSocket incomingSocket = null;
        DatagramSocket outgoingSocket = null;
        try {
            Charset charset = Charset.forName(CHARSET_NAME);
            String myHostname = InetAddress.getLocalHost().getHostName();
            String myIP = InetAddress.getLocalHost().getHostAddress();
            
            incomingSocket = new MulticastSocket(PORT);
            incomingSocket.joinGroup(InetAddress.getByName(MULTICAST_ADDR));
            byte[] incomingData = new byte[1024];
            DatagramPacket incomingPacket = new DatagramPacket(incomingData, incomingData.length);
            
            outgoingSocket = new DatagramSocket();
            byte[] outgoingData = (myHostname + " " + myIP).getBytes();
            DatagramPacket outgoingPacket = new DatagramPacket(outgoingData, outgoingData.length);
            outgoingPacket.setData(outgoingData);
            outgoingPacket.setPort(RESPONSE_PORT);
            
            while (true) {
                incomingSocket.receive(incomingPacket);
                InetAddress returnAddress = incomingPacket.getAddress();
                String dataReceived = new String(incomingPacket.getData(), 0, incomingPacket.getLength(), charset);
                logger.info("Received request from " + returnAddress.toString() + ": " + dataReceived);
                if (dataReceived.equals("*") || dataReceived.equals("") || dataReceived.equalsIgnoreCase(myHostname)) {
                    outgoingPacket.setAddress(returnAddress);
                    outgoingSocket.send(outgoingPacket);
                }
                logger.info("Sent response to " + returnAddress.toString() + ": " + new String(outgoingPacket.getData(), charset));
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally {
            incomingSocket.close();
            outgoingSocket.close();
        }
        
    }

}
