#include <converter.h>

#include <QScopedPointer>

int main(int argc, char* argv[])
{
	QScopedPointer<Converter> app(new Converter(argc, argv));
	return app->result;
}
