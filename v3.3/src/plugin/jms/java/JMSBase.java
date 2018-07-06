package jmsbase;

import java.util.Properties;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.BytesMessage;
import javax.jms.Queue;
import javax.jms.QueueConnection;
import javax.jms.QueueConnectionFactory;
import javax.jms.QueueSender;
import javax.jms.QueueReceiver;
import javax.jms.QueueSession;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;
import javax.jms.MapMessage;
import javax.jms.ObjectMessage;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import java.util.Enumeration;

import java.io.IOException;
import java.util.Date;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

class JMSBase {

	private Queue SendQueue = null;
	private Queue RecvQueue = null;
	private QueueConnection conn = null;
	private QueueSession session = null;
	private Message message = null;
	private QueueSender qSender = null;
	private QueueReceiver qReceiver = null;
	private QueueConnectionFactory connFactory = null;
	public static String JNDIFactory = null;
	public static String providerUrl = null;
	public static String connFactoryJNDI = null;
	public static String jSendQueue = null;
	public static String jRecvQueue = null;

	private int delivery = 0;
	private String corrID = null;
	private String mxsd_from = null;
	private String mxsd_to = null;
	private String mxsd_distinct_from = null;
	private String mxsd_locl_nodeid = null;

	public JMSBase(String JNDIFactory, String providerUrl, String connFactoryJNDI, String jSendQueue, String jRecvQueue) {

		this.JNDIFactory = JNDIFactory;
		this.providerUrl = providerUrl;
		this.connFactoryJNDI = connFactoryJNDI;
		this.jSendQueue = jSendQueue;
		this.jRecvQueue = jRecvQueue;	
	}

	private int init() {

		Context	ctx = null;

		Properties prop = new Properties();
		prop.put(Context.INITIAL_CONTEXT_FACTORY, JNDIFactory);
		prop.put(Context.PROVIDER_URL, providerUrl);

		if (jSendQueue == null && jRecvQueue == null) {
			System.out.println("send queue and recv queue all null!");
			return -1;
		}

		try {
			ctx = new InitialContext(prop);

		} catch (NamingException ne) {

			System.out.println("Init context error:" + ne);
			return -1;
		}

		if (jSendQueue != null) {

			try {
				connFactory = (QueueConnectionFactory) ctx.lookup(connFactoryJNDI);
				SendQueue = (Queue) ctx.lookup(jSendQueue);

			} catch (NamingException ne) {

				System.out.println("naming error:" + ne);
				return -1;
			}
		}

		if (jRecvQueue != null) {

			try {
				connFactory = (QueueConnectionFactory) ctx.lookup(connFactoryJNDI);
				RecvQueue = (Queue) ctx.lookup(jRecvQueue);

			} catch (NamingException ne) {

				System.out.println("naming error:" + ne);
				return -1;
			}
		}

		try {
			conn = (QueueConnection) connFactory.createConnection();
			session = conn.createQueueSession(false, Session.AUTO_ACKNOWLEDGE);

		} catch (JMSException je) {

			System.out.println("jms error:" + je);
			return -1;
		}

		return 0;
	}

	public int initSend() {

		if (SendQueue == null) {
			System.out.println("SendQueue is null!");
			return -1;
		}

		try {
			qSender = session.createSender(SendQueue);

		} catch (JMSException je) {
			System.out.println("jms error:" + je);
			return -1;
		}
		System.out.println("Create producer success!");

		return 0;
	}

	public int initRecv() {

		if (RecvQueue == null) {
			System.out.println("RecvQueue is null!");
			return -1;
		}

		try {
			qReceiver = session.createReceiver(RecvQueue);
			conn.start();
		} catch (JMSException je) {
			System.out.println("jms error:" + je);
			return -1;
		}
		System.out.println("Create consumer success!");

		return 0;
	}

	public int initRecv(String arg0) {

		if (RecvQueue == null) {
			System.out.println("RecvQueue is null!");
			return -1;
		}

		try {
			qReceiver = session.createReceiver(RecvQueue, arg0);
			conn.start();
		} catch (JMSException je) {
			System.out.println("jms error:" + je);
			return -1;
		}
		System.out.println("Create consumer success!");

		return 0;
	}

	public int sendTextMessage(String sendMsg) {

		try {
			message = session.createTextMessage(sendMsg);
			qSender.send(message);
		} catch (JMSException je) {
			System.out.println("jms error:" + je);
			return -1;
		}

		return 0;
	}

	public int sendBytesMessage(String sendMsg) {

		try {
			byte[] bytes = sendMsg.getBytes();
			BytesMessage msg = session.createBytesMessage();
			msg.writeBytes(bytes);

			if (corrID != null) {
				msg.setJMSCorrelationID(corrID); 
			}

			if (mxsd_from != null) {
				msg.setStringProperty("MXSD_FROM", mxsd_from);
			}

			if (mxsd_to != null) {
				msg.setStringProperty("MXSD_TO", mxsd_to);
			}

			if (mxsd_distinct_from != null) {
				msg.setStringProperty("MXSD_DISTINCT_FROM", mxsd_distinct_from);
			}

			if (mxsd_locl_nodeid != null) {
				msg.setStringProperty("MXSD_LOCAL_NODEID", mxsd_locl_nodeid); 
			}

			if (delivery == 1) {
				msg.setStringProperty("JMSXDeliveryCount", "1"); 
			}

			msg.setJMSCorrelationID("SmartIntegrator2_SmartIntegrator2_" + System.currentTimeMillis() + "_9999");
			msg.setStringProperty("MXSD_FROM", "SmartIntegrator2");
			msg.setStringProperty("MXSD_TO", "SVR_CARD");
			msg.setStringProperty("MXSD_DISTINCT_FROM", "SmartIntegrator2_SmartIntegrator2");
			msg.setStringProperty("MXSD_LOCAL_NODEID", "3000");
			msg.setStringProperty("JMSXDeliveryCount", "1");


			qSender.send(msg);
		} catch (JMSException je) {
			System.out.println("jms error:" + je);
			return -1;
		}
		finally{
			resetMsgAttr();
		}

		return 0;
	}

	public String recvMessage(int timeout) {

		String	text = null;
		try {
			if (timeout > 0) {
				message = qReceiver.receive(timeout);
			}
			else {
				message = qReceiver.receive();
			}

			if (message != null) {
				String corID = message.getJMSCorrelationID();
				if (message instanceof TextMessage){
					TextMessage textMessage = (TextMessage)message;
					//System.out.println("收到一条Text消息:"+textMessage.getText());
				} else if(message instanceof MapMessage ){
					System.out.println("收到一条Map消息:Recv message CorrelationID:" + corID);
				} else if(message instanceof StreamMessage ){
					System.out.println("收到一条Text消息:Recv message CorrelationID:" + corID);
				} else if(message instanceof BytesMessage ){
					//System.out.println("收到一条Bytes消息:Recv message CorrelationID:" + corID);
					byte[] block = new byte[(int) ((BytesMessage) message).getBodyLength()];
					((BytesMessage) message).readBytes(block);
					text = new String(block);
					//System.out.println("Recv bytes:" + text);
				}else if(message instanceof ObjectMessage ){
					System.out.println("收到一条Object消息Recv message CorrelationID:" + corID);
				}else {
					System.out.println("Unkown message type!");
				}
			}else{
				return null;
			}

		} catch (JMSException je) {
			System.out.println("jms error:" + je);
			return null;
		}
		return text;
	}

	public void setMsgCorrID(String corrID) {
		this.corrID = corrID;
	}

	public void setMsgFrom(String mxsd_from) {
		this.mxsd_from = mxsd_from;
	}

	public void setMsgTo(String mxsd_to) {
		this.mxsd_to = mxsd_to;
	}

	public void setMsgDistFrom(String mxsd_distinct_from) {
		this.mxsd_distinct_from = mxsd_distinct_from;
	}

	public void setMsgNodeID(String mxsd_locl_nodeid) {
		this.mxsd_locl_nodeid = mxsd_locl_nodeid;
	}

	public void setDelivery(int delivery) {
		this.delivery = delivery;
	}

	public void resetMsgAttr() {

		this.corrID = null;
		this.mxsd_from = null;
		this.mxsd_to = null;
		this.mxsd_distinct_from = null;
		this.mxsd_locl_nodeid = null;
		this.delivery = 0;
	}

	public void close() {
		try {
			qReceiver.close();
			qSender.close();
			session.close();
			conn.close();
		}
		catch (JMSException e) {
			e.printStackTrace();
		}
	}
}

