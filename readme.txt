1\��Ϊ����ģ��client���ͷ�����logic���߼���������ģ��Ϊcommon��server��
2\server֧��http��tcp��websocket���֣�Ĭ�϶˿ڷֱ�Ϊ8888,8886��
3\ʵ���߼���client����logic��logicͨ��http����status������status����״̬��redis������ظ�
	��¼������status֪ͨlogic��Ӧsession�˳����ӡ�
4\����Ѿ���¼��������ʵ����������Ϣ��ת����logic��ͨ��tcp���͡����͵�ַ���£�
	http://localhost:8880/pushMsg?member_id=&channel=&type=&logic_thread=&msg_body=
5\��־�����Ĭ��Ϊ/tmp/master.log /tmp/client.log
6\���������ļ���Ĭ�� �����ļ���=������+.cf����master_server.cf

7\src/thrift_protosĿ¼��Ϊ����˺Ϳͻ��� Э�鶨���ļ���ͨ��
	thrift -r --gen cpp -o ./ StateSvr.thrift ���ɴ����ļ�
8\src/thrift_srvΪ����˴����ļ����޸�svr_job.h svr_job.cpp���ɱ�������thrift����
9\src/thrift_protosĿ¼�´��ThriftProxy.cpp �û�����thrift����

cmd seq
reload config
cmdid ip port ha
thriftģ����ӽ�ȥ

libgo

