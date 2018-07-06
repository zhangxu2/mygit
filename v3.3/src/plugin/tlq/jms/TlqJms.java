package tlqjms;

import javax.jms.*;
import javax.naming.*;
import com.tongtech.org.apache.log4j.*;

class TlqJms {

	private Queue SendQueue = null;
	private Queue RecvQueue = null;
	private Connection conn = null;
	private Session session = null;
	private Message message = null;
	private MessageProducer producer = null;
	private	MessageConsumer consumer = null;
	public static String url = null;
	public static String factory = null;
	public static String jSendQueue = null;
	public static String jRecvQueue = null;
	public static final String tcf = "tongtech.jms.jndi.JmsContextFactory";
	private static Logger logger = Logger.getLogger(TlqJms.class);
	
	public TlqJms(String url, String factory, String jsendQ, String jrecvQ) {
		this.url = url;
		this.factory = factory;
		this.jSendQueue = jsendQ;
		this.jRecvQueue = jrecvQ;
	}

	private int init() {

		ConnectionFactory cf = null;
		java.util.Properties prop = new java.util.Properties();
		prop.put("java.naming.factory.initial", tcf);
		prop.put("java.naming.provider.url", url);
	
		if (jSendQueue == null && jRecvQueue == null) {
			logger.error("send queue and recv queue all null!");
			return -1;
		}
		Context ictx = null;
		try {
			ictx = new InitialContext(prop);

		} catch (NamingException ne) {

			logger.error("Init context error:" + ne);
			return -1;
		}
	
		if (jSendQueue != null) {

			try {
				cf = (ConnectionFactory) ictx.lookup(factory);
				SendQueue = (Queue) ictx.lookup(jSendQueue);

			} catch (NamingException ne) {

				logger.error("naming error:" + ne);
				return -1;
			}
		}

		if (jRecvQueue != null) {

			try {
				cf = (ConnectionFactory) ictx.lookup(factory);
				RecvQueue = (Queue) ictx.lookup(jRecvQueue);

			} catch (NamingException ne) {

				logger.error("naming error:" + ne);
				return -1;
			}
		}

		try {
			conn = cf.createConnection();
			session = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);

		} catch (JMSException je) {

			logger.error("jms error:" + je);
			return -1;
		}
	
		return 0;
	}

	public int initSend() {
		
		if (SendQueue == null) {
			logger.error("SendQueue is null!");
			return -1;
		}
		
		try {
			producer = session.createProducer((Destination) SendQueue);

		} catch (JMSException je) {
			logger.error("jms error:" + je);
			return -1;
		}
		logger.error("Create producer success!");

		return 0;
	}
	
	public int initRecv() {
		
		if (RecvQueue == null) {
			logger.error("RecvQueue is null!");
			return -1;
		}

		try {
			consumer = session.createConsumer((Destination) RecvQueue);
			conn.start();
		} catch (JMSException je) {
			logger.error("jms error:" + je);
			return -1;
		}
		logger.error("Create consumer success!");

		return 0;
	}

	public int sendMessage(String sendMsg) {

		try {
			message = session.createTextMessage(sendMsg);
			producer.send(message);
		} catch (JMSException je) {
			logger.error("jms error:" + je);
			return -1;
		}
			
		return 0;
	}

	public String recvMessage(int timeout) {

		String	text = null;
		try {
			if (timeout > 0) {
				message = consumer.receive(timeout);
			}
			else {
				message = consumer.receive();
			}
			if (message instanceof TextMessage) {
				text = ((TextMessage) message).getText();
			}
		} catch (JMSException je) {
			logger.error("jms error:" + je);
			return null;
		}
		return text;
	}
}

