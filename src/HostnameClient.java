package net.billchatfield.apps.hostnameclient;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.MulticastSocket;

import javax.swing.JFrame;

import net.billchatfield.apps.hostnameservice.HostnameService;

public class HostnameClient {

    /**
     * @param args
     */
    public static void main(String[] args) {
        MulticastSocket outgoingSocket = null;
        DatagramSocket incomingSocket = null;
        try {
            outgoingSocket = new MulticastSocket();
            InetAddress multicastGroupAddr = InetAddress.getByName(HostnameService.MULTICAST_ADDR);
            outgoingSocket.joinGroup(multicastGroupAddr);
            if (args.length == 0) {
                args = new String[] {"*"};
            }
            for (String arg : args) {
                byte[] outgoingData = arg.getBytes(HostnameService.CHARSET_NAME);
                DatagramPacket outgoingPacket = new DatagramPacket(outgoingData, outgoingData.length);
                outgoingPacket.setPort(HostnameService.PORT);
                outgoingPacket.setAddress(multicastGroupAddr);
                outgoingSocket.send(outgoingPacket);
                    
                incomingSocket = new DatagramSocket(HostnameService.RESPONSE_PORT);
                byte[] incomingData = new byte[1024];
                DatagramPacket incomingPacket = new DatagramPacket(incomingData, incomingData.length);
                incomingSocket.receive(incomingPacket);
                String result = new String(incomingPacket.getData(),0,incomingPacket.getLength(),HostnameService.CHARSET_NAME);
                System.out.println(result);
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
