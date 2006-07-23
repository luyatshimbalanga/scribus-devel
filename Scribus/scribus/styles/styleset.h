

#ifndef STYLESET_H
#define STYLESET_H

#include <assert.h>
#include <qvaluelist.h>

template<class STYLE>
class StyleSet {
public:
	STYLE& operator[] (uint index) { assert(index < styles.count()); return * styles[index]; }
	const STYLE& operator[] (size_t index) const { assert(index < styles.count()); return * styles[index]; }
	inline int find(QString name);
	uint count() const { return (uint) styles.count(); }
	void append(STYLE* style) { styles.append(style); }
	inline void remove(uint index);
	inline void redefine(QValueList<STYLE> defs, bool removeUnused=false);
	void create(const STYLE& proto) { styles.append(new STYLE(proto)); }
	StyleSet() : styles() {}
	~StyleSet() { while(styles.count()>0) remove(0); }
private:
	StyleSet(const StyleSet&) {}
	StyleSet& operator= (const StyleSet&) { return *this; }
	QValueList<STYLE*> styles;
};

template<class STYLE>
inline void StyleSet<STYLE>::remove(uint index)
{
	assert(index < styles.count()); 
	typename QValueList<STYLE*>::Iterator it = styles.at(index);
	delete *it;
	styles.remove(it);
}

template<class STYLE>
inline int StyleSet<STYLE>::find(QString name)
{
	for (uint i=0; i < styles.count(); ++i)
		if (styles[i]->name() == name)
			return i;
	return -1;
}

template<class STYLE>
inline void StyleSet<STYLE>::redefine(QValueList<STYLE> defs, bool removeUnused)
{
	for (int i=signed(styles.count())-1; i >=0; --i) {
		bool found = false;
		for (uint j=0; j < defs.count(); ++j) {
			if (styles[i]->name() == defs[j].name()) {
				found = true;
				(*styles[i]) = defs[j];
				break;
			}
		}
		if (!found && removeUnused) {
			remove(i);
		}
	}
	for (uint j=0; j < defs.count(); ++j) {
		if (find(defs[j].name()) < 0) {
			create(defs[j]);
		}
	}
}

#endif


